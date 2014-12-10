#include <string.h>
#include "general.h"
#include "error.h"
#include "weights.h"

extern  int     nn;          /* number of nodes */
extern  int     ni;          /* number of inputs */
extern  int     nt;          /* nn + ni + 1 */

extern  long    tsweeps;     /* total sweeps */
extern  long    sweep;       /* current sweep */

extern  float   **wt;        /* (nn x nt): weights */

extern  char    loadfile[];  /* .wts file to start with */
extern  char    root[];      /* root file name */

extern CFStruct **cinfo;     /* Used to look at connection flags. */


/* *********************************** */
extern int bptt;        /* bptt flag. */
extern int copies;      /* number of network copies. */
extern int true_nn;     /* Number of original nodes. */
extern int true_ni;     /* Number of original inputs. */
extern int true_nt;     /* Number of total nodes. */
/* *********************************** */


int save_wts(void) 
{
    FILE        *fp;

    register    int     i;
    register    int     j;

    float       *w;
    float       **wp;

    char        file[128];

    /* ************************ */
    CFStruct *ci;
    /* ************************ */

    sprintf(file, "%s.%ld.wts", root, sweep);
    if ((fp=fopen(file, "w")) == NULL) {
        report_condition("Can't create weights file.", 2);
        return 1;
    }

    fprintf(fp, "NETWORK CONFIGURED BY TLEARN\n");
    fprintf(fp, "# weights after %ld sweeps\n", sweep);
    fprintf(fp, "# WEIGHTS\n");

    /* to each node */
    wp = wt;
    /* ********************************************** */
    /* Code modified to support bptt
       weight saving. If bptt flag is
       set, then we need to recompress
       the feed forward network into
       its original form, which is much
       more involved then normal weight
       saving; hence the longer code
       in bptt's case. */


    /* DEBUG */
    /*
       fprintf(fp," first, showing unfolded weight matrix.\n");
       for(i=0;i<nn;i++,wp++) {
           w = *wp;
           fprintf(fp,"N%i      ",i+1);
           for(j=0;j<nn+ni+1;j++,w++) {
               if (*w>=0.0) fprintf(fp," %f ",*w);
               else fprintf(fp,"%f ",*w);
           }
           fprintf(fp,"\n");
       }
       wp = wt;
       fprintf(fp," now, showing cinfo structure.\n");
       for(i=0;i<nn;i++) {
           ci = *(cinfo+i);
           fprintf(fp,"N%i      ",i+1);
           for(j=0;j<nn+ni+1;j++,ci++) {
               fprintf(fp,"%i ",ci->rec);
           }
           fprintf(fp,"\n");
       }
       */

    if (!bptt) {                /* NOTE: old Tlearn code follows: */
        for (i = 0; i < nn; i++, wp++){
            fprintf(fp, "# TO NODE %d\n",i+1);
            w = *wp;
            /* from each bias, input, and node */
            for (j = 0; j < nt; j++,w++){
                fprintf(fp,"%f\n",*w);
            }
        }
    }
    else {                      /* NOTE: new bptt code follows: */
        /* All we want to do here is not actually print
           out all of the weights, because our feed-forward
           unfolding of the bptt network is not the same,
           structurally speaking, as the original. So,
           we just want to print out the weights that
           are important for the folding of the network.
           Since all weight copies are identical, we can
           simply print out the last copy of the network
           along with its associated bias and input connections. */

        /* Get to last copy's weights. */
        for (i = 0; i < true_nn; i++) {
            wp = wt + ((copies-1)*true_nn) + i;
            /* Note that we look at the FIRST copy
               for the ci->rec info, since that's
               the only copy which retains the manual
               recurrence info. */
            fprintf(fp, "# TO NODE %d\n",(i+1));
            w = *wp;
            /* First, write bias weight. */
            fprintf(fp,"%f\n",*w);
            /* Now, write input weights. */
            /* Get to last copy's inputs. */
            w++;
            w = w + ((copies-1)*true_ni);
            for (j = 0; j < true_ni; j++) {
                fprintf(fp,"%f\n",*w);
                w++;
            }
            /* Finally, write node weights. We need to be
               careful here, because this is where refolding
               really takes place. Whenever we see a recurrent
               connection (marked by a 2), we have to
               look at the previous copy connections to get
               the actual weights. */

            w = w + ((copies-1)*true_nn);
            ci = *(cinfo + i) + ni + 1;
            for (j = 0; j < true_nn; j++) {
                if(ci->rec==2) {
                    fprintf(fp,"%f\n",*(w - true_nn));
                }
                else {
                    fprintf(fp,"%f\n",*w);
                }
                w++;
                ci++;
            }
        }

    }

    fflush(fp);
    fclose(fp);

    return 0;
}


int load_wts(void)
{

    FILE        *fp;

    register    int     i;
    register    int     j;

    register    float   *w;
    /* ************************ */
    register    float   *copy_w;
    /* ************************ */
    register    float   **wp;

    int tmp;

    char        mode[10];

    /* ************************ */
    CFStruct *ci;
    /* ************************ */

    if ((fp=fopen(loadfile, "r")) == NULL) {
        report_condition("Can't access weights file.", 2);
        return 1;
    }
    fscanf(fp, "NETWORK CONFIGURED BY %s\n", mode);
    if (strcmp(mode, "TLEARN") != 0)
        return report_condition("Weights file are not for tlearn-configured network.",2);

    fscanf(fp, "# weights after %ld sweeps\n", &tsweeps);
    fscanf(fp, "# WEIGHTS\n");

    /* to each of nn nodes */
    wp = wt;
    /* **************************************** */
    /* Code modified so that if bptt
       flag is set, we don't actually try
       to read in as many weights as we
       have connections in our network (since
       our network is now much bigger than
       the user actually specified). We'll
       just need to get as many weights as
       the user should specify. */

    if (!bptt) {                /* NOTE: following is old Tlearn code: */
        for (i = 0; i < nn; i++, wp++){
            fscanf(fp, "# TO NODE %d\n",&tmp);
            w = *wp;
            /* from each bias, input, and node */
            for (j = 0; j < nt; j++, w++){
                fscanf(fp,"%f\n",w);
            }
        }
    }
    else {                      /* NOTE: following is new bptt code: */
        for (i = 0; i < true_nn; i++, wp++){
            fscanf(fp, "# TO NODE %d\n",&tmp);
            w = *wp;

            /* First, get bias connection. */
            fscanf(fp,"%f\n",w++);
            /* Now, get input connections. */
            for (j = 0; j < true_ni; j++, w++) {
                fscanf(fp,"%f\n",w);
            }

            /* Now, SKIP over the rest of the inputs (for other copies) */
            w = w + (ni-true_ni);

            /* Load in node connections. */
            for (j = 0; j < true_nn; j++, w++) {
                fscanf(fp,"%f\n",w);
            }
        }
        /* Finally, we need to move all recurrent
           connections to the next level. */
        for (i = 0; i < true_nn; i++) {
            w = *(wt+i);
            ci = *(cinfo+i);
            w = w + ni + 1;
            ci = ci + ni + 1;
            for(j=0;j<true_nn;j++,w++,ci++) {
                if(ci->rec==2) {
                    copy_w = (*(wt+i+true_nn)+ni+1+j);
                    *copy_w = *w;
                    *w = 0.0;
                }
            }
        }
        /* DEBUG */
        /*
           printf("Loaded in this weight table (pre-duplication):\n");
           for (i = 0; i < nn; i++) {
           w = *(wt+i);
           ci = *(cinfo+i);
           printf("Node #%i     ",i+1);
           for(j=0;j<nn+ni+1;j++,w++,ci++) {
           if(*w>=0) printf(" %1.3f(%i) ",*w,ci->rec);
           else printf("%1.3f(%i) ",*w,ci->rec);
           }
           printf("\n");
           }
           */
    }
    /* **************************************** */

    fclose(fp);
    return 0;
}


