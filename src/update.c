#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "general.h"
#include "error.h"
#include "parse.h"  /* for get_nums() */
#include "update.h"

extern long GetFileModTime(char *buf);

extern  int     nn;             /* number of nodes */
extern  int     ni;             /* number of inputs */
extern  int     no;             /* number of outputs */
extern  int     nt;             /* nn + ni + 1 */
extern  int     np;             /* ni + 1 */
extern  int     ngroups;        /* number of groups */
extern  int     randomly;       /* flag for presenting inputs in random order */
extern  int     localist;       /* flag for localist inputs */
extern  int     limits;         /* flag for limited weights */

extern  char    root[128];      /* root filename for .data, .teach, etc. files */

extern  float   rate;           /* learning rate */
extern  float   momentum;       /* momentum */

/* ***************************************** */
extern int      bptt;           /* bptt flag */
extern int      copies;         /* number of network copies. */
extern int      true_ni;        /* original input count. */
extern int      true_nn;        /* original node count. */
extern int      true_no;        /* original output node count. */
/* ***************************************** */

extern CFStruct **cinfo;       /* (nn x nt) connection info */


static char    buf[128];

static int     *ldata = 0;

static long    *ntimes;
static long    dc = 0;
static long    dn;
static long    ds;

static float   *data = 0;
static float   *otarget = 0;    /* (no) back-up copy of target values */
static float   *teach;
static float   *dm;

/* ***************************************** */
static float   wi_sum;         /* Used to sum weight's. */
/* ***************************************** */


int init_inputs(long *maxtime)
{
    static int *ld;
    int j;
    int *idata, *id, *lld;
    long ii, modtime;
    register float   *d;
    FILE *fpdata;

    static long modtime_save;   /* These are used to see if the state */
    static int  ni_save;        /* of the network inputs has changed. */

    dc= 0;
    dm= data;

    /* get .data file */
    sprintf(buf, "%s.data", root);

    modtime= GetFileModTime(buf);
    if (ni == ni_save && modtime == modtime_save) return 0;

    fpdata= fopen(buf, "r");
    if (fpdata == NULL) {
        sprintf(buf, "The data file \"%s.data\" is empty or missing.", root);
        return report_condition(buf, 2);
    }
    parse_err_set(fpdata, buf, "Data");

    /* determine format of .data file */
    fscanf(fpdata,"%s",buf);
    localist= 0;
    if (strcmp(buf, "localist") == 0){
        localist = 1;
    }
    else if (strcmp(buf, "distributed") != 0) {
        parse_err("Data file must be localist or distributed.");
        fclose(fpdata);
        return -1;
    }

    /* determine size of .data file */
    if (fscanf(fpdata,"%ld",maxtime) != 1) {
        parse_err("Data file doesn't specify number of items.");
        fclose(fpdata);
        return -1;
    }

    /* calloc space for data */

    /* *********************************** */
    /* When the bptt flag is set, the data is read in
       and also preceded by the last (copies) number
       of inputs. In other words, the data file will
       only have true_ni number of inputs, but the
       update algorithm will look for ni inputs. So
       the inputs are effectively shifted left and
       the new input tacked on to the right for each
       time click. For instance, suppose we've just
       read in the sequence "a","b", and "c" (for a
       three-copy bptt network), and we're about to
       read "d". Before reading, the last input vector
       looks like "abc". After reading, it looks like
       "bcd". This is how the sequences are implemented
       without requiring the user to explicitly state
       them. Understanding the bptt code for localist
       data encoding is not hard if you understand how
       localist encoding works in the non-bptt case,
       which IS hard to understand! */

    if (!bptt) {
        if (localist){
            dn = *maxtime;
            ds = *maxtime * ni;

            if (ldata) free(ldata);
            ldata= NULL;
            ldata = (int *) calloc(ds, sizeof(int));
            if (ldata == NULL) {
                fclose(fpdata);
                return memerr("ldata");
            }

            idata = (int *) calloc((ni+1), sizeof(int));
            if (idata == NULL) {
                fclose(fpdata);
                return memerr("idata");
            }

            /* read data */
            ld = ldata;
            for (ii = 0; ii < dn; ii++){
                fscanf(fpdata,"%s",buf);
                if (get_nums(buf,ni,0,idata)) {
                    free(idata);
                    fclose(fpdata);
                    return -1;
                }
                id = idata + 1;
                lld = ld;
                for (j = 1; j <= ni; j++, id++){
                    if (*id)
                        *lld++ = j;
                }
                *lld = 0;
                ld += ni;
            }
            free(idata);
        }
        else {
            dn = *maxtime;
            ds = *maxtime * ni;
            if (data) free(data);
            data= NULL;
            data = (float *) calloc(ds, sizeof(float));
            if (data == NULL) {
                fclose(fpdata); 
                return memerr("data array");
            }

            /* read data */
            d = dm = data;
            for (ii = 0; ii < ds; ii++, d++){
                if (fscanf(fpdata,"%s",buf) != 1) {
                    parse_err("Unexpected end of file.");
                    fclose(fpdata);
                    return -1;
                }
                *d = atof(buf);
                if ((*d == 0.) && (buf[0] != '0') && (buf[1] != '0')) {
                    parse_err("Bad numeric value or other error.");
                    fclose(fpdata);
                    return -1;
                }
                                
            }
        }
    }
    else {                      /* NOTE: this is new bptt code... */
        if (localist) {
            parse_err("Localist encoding not supported with BPTT.");
            fclose(fpdata);
            return -1;


            /* NOT COMPLETE *
            dn = *maxtime;
            ds = *maxtime * ni;
            if (ldata) free(ldata);
            ldata= NULL;
            ldata= (int *) calloc(ds, sizeof(int));
            if (ldata == NULL) {
                fclose(fpdata); 
                return memerr("ldata array");
            }
               
            idata = (int *) calloc((ni+1), sizeof(int));
            if (idata == NULL) {
                fclose(fpdata); 
                return memerr("idata array");
            }
               
            ld = ldata;
            for (ii = 0; ii < dn; ii++){
            }
            free(idata);
            */

        }
        else {
            dn = *maxtime;
            ds = *maxtime * ni;
            if (data) free(data);
            data= NULL;
            data= (float *) calloc(ds, sizeof(float));
            if (data == NULL) {
                fclose(fpdata); 
                return memerr("data array");
            }

            /* read data */
            d = dm = data;
            for (ii = 0; ii < (*maxtime); ii++){
                for (j = 0;j < (ni - true_ni);j++) {
                    if ((d - ni + true_ni) < data) {
                        *d = 0.0;
                    }
                    else {
                        *d = *(d - ni + true_ni);
                    }
                    d++;
                }
                for (j = 0;j < (true_ni); j++) {
                    fscanf(fpdata,"%s",buf);
                    *d = atof(buf);
                    if ((*d == 0.) && (buf[0] != '0') && (buf[1] != '0')) {
                        parse_err("Bad numeric value or other error.");
                        fclose(fpdata);
                        return -1;
                    }
                    d++;
                }

            }
        }
    }

    /* ************************************* */
    /* DEBUG */
    /* Show input vector */
    /*
       if(!localist) {
           printf("Input Vector (distributed):\n");
           j = 0;
           k = 0;
           for(i=0;i<ds;i++) {
               printf("%1.1f ",*(data+i));
               j++;
               k++;
               if(j==true_ni) {printf("   ");j=0;}
               if(k==ni) {printf("\n");k=0;}
           }
           printf("\n");
       }
       else {
           printf("Input Vector (localist):\n");
           j = 0;
           k = 0;
           for(i=0;i<ds;i++) {
               printf("%i ",*(ldata+i));
               j++;
               k++;
               if(j==true_ni) {printf("   ");j=0;}
               if(k==ni) {printf("\n");k=0;}
           }
           printf("\n");
       }
    */

    fclose(fpdata);
    ni_save= ni;
    modtime_save= modtime;

    return NO_ERROR;
}




int update_inputs(float *aold, int tick, int flag, long *maxtime, int **local)
{
    register        int     i;
    register        float   *d;
    register        float   *zo;

    /* update input (only at major time increments) */
    if (tick == 0) {
        /* read next ni inputs from .data file */
        if (localist){
            if (randomly){
                dc = (rand() >> 4) % dn;
                if (dc < 0)
                    dc = -dc;
                *local = (int *) (ldata + dc * ni);
            }
            else {
                *local = (int *) (ldata + dc * ni);
                if (++dc >= dn)
                    dc = 0; 
            }
        }
        else {
            if (randomly){
                dc = (rand() >> 4) % dn;
                if (dc < 0)
                    dc = -dc;
                d = (float *) (data + dc * ni);
                zo = aold + 1;
                for (i = 0; i < ni; i++, zo++, d++){
                    *zo = *d;
                }
            }
            else {
                d = dm;
                zo = aold + 1;
                for (i = 0; i < ni; i++, zo++, d++, dc++){
                    if (dc >= ds){
                        dc = 0;
                        d = data;
                    }
                    *zo = *d;
                }
                dm = d;
            }
        }
    }
    else {
        /* turn off input during extra ticks with -I */
        if (flag){
            zo = aold + 1;
            for (i = 0; i < ni; i++, zo++)
                *zo = 0.;
        }
    }
    return NO_ERROR;
}



static long len;
static long *nm;
static float *tm;


int init_targets(void)
{
    int k;
    int local;                  /* flag for localist output */
    long i, ts, modtime;
    register float *t;
    register long *n;
    register int j;
    FILE *fpteach;
        
    static long modtime_save;   /* These are used to see if the state */
    static int  no_save;        /* of the network outputs has changed. */

    /* get .teach file */
    sprintf(buf, "%s.teach", root);
    modtime= GetFileModTime(buf);
    if (no == no_save && modtime == modtime_save) return 0;

    fpteach= fopen(buf, "r");
    if (fpteach == NULL) {
        sprintf(buf, "The teach file \"%s.teach\" is empty or missing.", root);
        return report_condition(buf, 2);
    }

    parse_err_set(fpteach, buf, "Teach");

    /* calloc space for back-up copy of targets */
    if (otarget) free(otarget);
    otarget= NULL;
    otarget= (float *) calloc(no, sizeof(float));
    if (otarget == NULL) {
        fclose(fpteach);
        return memerr("otarget");
    }

    /* determine format of .teach file */
    fscanf(fpteach, "%s", buf);
    if (strcmp(buf, "localist") == 0) local = 1;
    else if (strcmp(buf, "distributed") == 0) local= 0;
    else {
        parse_err("File must be localist or distributed.");
        fclose(fpteach);
        return -1;
    }
        
    /* determine size of teach array */
    if (fscanf(fpteach, "%ld", &len) != 1) {
        parse_err("File doesn't specify number of items.");
        fclose(fpteach);
        return -1;
    }

    /* calloc space for teach and ntimes buffers */
    ts = len * no;
    if (teach) free(teach);
    teach= NULL;
    teach= (float *) calloc(ts, sizeof(float));
    if (teach == NULL) {
        fclose(fpteach);
        return memerr("teach array");
    }

    if (ntimes) free(ntimes);
    ntimes= NULL;
    ntimes= (long *) calloc(len, sizeof(long));
    if (ntimes == NULL) {
        fclose(fpteach); 
        return memerr("ntimes array");
    }

    /* read teach info */
    t = tm = teach;
    n = nm = ntimes;
    if (!bptt) {
        for (i = 0; i < len; i++, n++){
            if (local){
                fscanf(fpteach,"%s",buf);
                k = atoi(buf) - 1;
                if (k < 0) {
                    parse_err("Bad numeric value or other error.");
                    fclose(fpteach);
                    return -1;
                }
                for (j = 0; j < no; j++, t++){
                    if (j == k)
                        *t = 1.;
                    else
                        *t = 0.;
                }
            }
            else {
                for (j = 0; j < no; j++, t++){
                    fscanf(fpteach,"%s",buf);
                    /* asterick is don't care sign */
                    if (buf[0] == '*')
                        *t = -9999.0;
                    else {
                        *t = atof(buf);
                        if ((*t == 0.) && (buf[0] != '0') && (buf[1] != '0')) {
                            parse_err("Bad numeric value or other error.");
                            fclose(fpteach);
                            return -1;
                        }
                                                
                    }
                }
            }
        }
    }
    else {                      /* New bptt code (output windowing 5.19.93) */
        /* Essentially, we're now doing for the output array
           what we did for the input array. It's sort of a
           raging debate as to whether or not outputs should
           be windowed, or whether we should propagate error
           only from the current time-step output. We used to
           only propagate error from the current output. Anyway,
           we're going ahead with it. All that should change:
           [1] (no) should reflect outputs*copies, not just
           number of outputs as it used to, since there
           really are several outputs now.
           [2] All this code here. There should be a file around
           somewhere with the extension "no_output_windowing"
           which is just the old version of this file,
           albeit without this code.
           */
        for (i = 0; i < len; i++, n++){
            if (local){
                parse_err("BPTT not supported with Localist teach files.");
                fclose(fpteach);
                   return -1;

                /*
                   fscanf(fpteach,"%s",buf);
                   k = atoi(buf) - 1;
                   if (k < 0) {
                        parse_err("Bad numeric value or other error.");
                        fclose(fpteach);
                        return -1;
                    }
                   for (j = 0; j < no; j++, t++){
                   if (j == k)
                   *t = 1.;
                   else
                   *t = 0.;
                   }
                   */
            }
            else {
                for (j = 0; j < (no-true_no); j++, t++) {
                    if((t - no + true_no)<teach) {
                        *t = 0.0;
                    }
                    else {
                        *t = *(t-no+true_no);
                    }
                }
                for (j = 0; j < true_no; j++, t++){
                    fscanf(fpteach,"%s",buf);
                    /* asterick is don't care sign */
                    if (buf[0] == '*')
                        *t = -9999.0;
                    else {
                        *t = atof(buf);
                        if ((*t == 0.) && (buf[0] != '0') && (buf[1] != '0')) {
                            parse_err("Bad numeric value or other error.");
                            fclose(fpteach);
                            return -1;
                        }
                    }
                }
            }
        }
    }

    if (fscanf(fpteach,"%s",buf) != EOF)  /* see if we're at the end of the file */
        report_condition("Warning: The .teach file contains extraneous information.  "
                         "Make sure there are no timestamps in the .teach file.  ", 1);

    /*DEBUG*/
    /* show output vector */
    /*
    if(!local) {
        printf("Output Vector (distributed):\n");
        j = 0; k = 0;
        for(i=0;i<ts;i++) {
            printf("%1.1f ",*(teach+i));
            k++;
            j++;
            if(j==true_no) {printf("   ");j=0;}
            if(k==no) {printf("\n");k=0;}
        }
        printf("\n");
    }
    else {
        printf("Output Vector (localist):\n");
        j = 0; k = 0;
        for(i=0;i<ts;i++) {
            printf("%1.1f ",*(teach+i));
            k++;
            j++;
            if(j==true_no) {printf("   ");j=0;}
            if(k==no) {printf("\n");k=0;}
        }
        printf("\n");
    }
    */

    fclose(fpteach);
    no_save= no;
    modtime_save= modtime;

    return NO_ERROR;
}





int update_targets(float *atarget, long time, int tick, int flag, long *maxtime)
{
    register    int     j;

    register    float   *ta;
    register    float   *t;
    register    long    *n;
    register    float   *to;

    static      long    nc = 0;
    static      long    next;   /* next time tag in .teach file */

    /* check for new target values (only at major time increments) */
    if (tick == 0){
        t = tm;
        n = nm;
        /* restore previous values if destroyed by -T */
        if (flag){
            ta = atarget;
            to = otarget;
            for (j = 0; j < no; j++, ta++, to++)
                *ta = *to;
        }

        /* if inputs are selected randomly, time-tags are
           assumed to run sequentially, and targets are
           selected to match input */

        if (randomly){
            if (dc >= len)
                return report_condition("Random ordering requires same number of data/teacher vectors.", 2);

            ta = atarget;
            t = (float *) (teach + no * dc);
            for (j = 0; j < no; j++, ta++, t++){
                *ta = *t;
            }
            return NO_ERROR;
        }

        /* rewind whenever .data begins again at time 0 */
        if (time == 0){
            nc = 0;
            t = teach;
            n = ntimes;
            next = *n;
            ta = atarget;
            for (j = 0; j < no; j++, ta++)
                *ta = -9999.0;
        }
        /* get new target values when time matches next */
        if (time >= next){
            /* read next no targets */
            ta = atarget;
            for (j = 0; j < no; j++, t++, ta++){
                *ta = *t;
            }
            /* final target persists till end of input */
            n++;
            if (++nc >= len)
                next = *maxtime;
            else
                next = *n;
        }
        tm = t;
        nm = n;
        /* remember target values if -T will destroy them */
        if (flag){
            ta = atarget;
            to = otarget;
            for (j = 0; j < no; j++, ta++, to++)
                *to = *ta;
        }
    }
    else {
        /* turn off target during extra ticks with -T */
        if (flag){
            ta = atarget;
            for (j = 0; j < no; j++, ta++)
                *ta = -9999.0;
        }
    }

    return NO_ERROR;
}

static  long    *rtimes;
static long l;


int init_reset(void)
{
    long i;
    FILE *fpreset;
        
    /* get .reset file */
    sprintf(buf, "%s.reset", root);
    fpreset= fopen(buf, "r");
    if (fpreset == NULL) {
        sprintf(buf, "The reset file \"%s.reset\" is empty or missing.", root);
        return report_condition(buf, 2);
    }

    parse_err_set(fpreset, buf, "Reset");

    /* determine size of .reset file */
    if (fscanf(fpreset,"%ld",&l) != 1) {
        parse_err("First line should specify size of file.");
        fclose(fpreset);
        return -1;
    }

    /* calloc space for rtimes buffer */
    if (rtimes) free(rtimes);
    rtimes= NULL;
    rtimes= (long *) calloc(l, sizeof(long));
    if (rtimes == NULL) {
        fclose(fpreset); 
        return memerr("rtimes array");
    }

    /* read reset info */
    nm = rtimes;
    for (i = 0; i < l; i++, nm++)
        fscanf(fpreset,"%ld",nm);
    nm = rtimes;

    return NO_ERROR;
}        
       

int update_reset(long time, int tick, int flag, long *maxtime, int *now)
{
        static  long    next;           /* next time tag in .teach file */
        static  long    *nm;
        static  long    nc = 0;


        *now = 0;

        if (flag == 0) return NO_ERROR;

        /* check for new resets (only at major time increments) */
        if (tick == 0) {
                /* rewind whenever .data begins again at time 0 */
                if (time == 0){
                        nc = 0;
                        nm = rtimes;
                        next = *nm;
                }
                if (time >= next) {
                        *now = 1;
                        nm++;
                        if (++nc >= l)
                                next = *maxtime;
                        else
                                next = *nm;
                }
        }

        return NO_ERROR;
}


int update_weights(float **awt, float **adwt, float **awinc)
{
    register    int     i;
    register    int     j;

    register    CFStruct        *ci;

    register    float   *w;
    register    float   *dw;
    register    float   *wi;
    register    float   **wip;
    register    float   **wp;
    register    float   **dwp;

    register    CFStruct        **cp;

    register    int     k;
    register    int     n;
    register    float   *sum;

    float       asum;


    /* DEBUG */
    /*
       printf("Delta weight matrix (used to compute wi table):\n");
       cp = cinfo;
       wp = awt;
       wip = awinc;
       dwp = adwt;
       for (i = 0;i<nn;i++,cp++,wp++,wip++,dwp++) {
           ci = *cp;
           w = *wp;
           wi = *wip;
           dw = *dwp;
           printf("#%i  ",i+1);
           for(j=0;j<(nn+ni+1);j++,ci++,w++,wi++,dw++) {
               if(*dw<0.0) printf("%f ",*dw);
               else printf(" %f ",*dw);
           }
           printf("\n");
       }
    */


    /* update weights if they are not fixed */
    sum = &asum;
    cp = cinfo;
    wp = awt;
    dwp = adwt;
    wip = awinc;


    /* **************************** */
    /* bptt forces us to first sum the weight
       increments before applying them to the
       weights. */


    if (!bptt) {
        for (i = 0; i < nn; i++, cp++, wp++, dwp++, wip++){
            ci = *cp;
            w = *wp;
            dw = *dwp;
            wi = *wip;
            for (j = 0; j < nt; j++, dw++, wi++, w++, ci++){
                if ((ci->con) && !(ci->fix)){
                    *wi = rate * *dw + momentum * *wi;
                    *w += *wi;
                    *dw = 0.;
                }
            }
        }
    }
    else {
        /* I put this in a separate branch to maximize
           speed. The only difference here is that we
           don't adjust the weights here. */
        for (i = 0; i < nn; i++, cp++, dwp++, wip++){
            ci = *cp;
            dw = *dwp;
            wi = *wip;
            for (j = 0; j < nt; j++, dw++, wi++, /* w++,*/ ci++){
                if ((ci->con) && !(ci->fix)){
                    *wi = rate * *dw + momentum * *wi;
                    *dw = 0.;
                }
            }
        }
    }
    /* DEBUG */
    /*
       printf("Pre-incremented weight matrix:\n");
       cp = cinfo;
       wp = awt;
       wip = awinc;
       for (i = 0;i<nn;i++,cp++,wp++,wip++) {
           ci = *cp;
           w = *wp;
           wi = *wip;
           printf("#%i  ",i+1);
           for(j=0;j<(nn+ni+1);j++,ci++,w++,wi++) {
               if(*w<0.0) printf("%f ",*w);
               else printf(" %f ",*w);
           }
           printf("\n");
       }
       */
    /* DEBUG */
    /*
       printf("Pre-summed weight increment matrix:\n");
       cp = cinfo;
       wp = awt;
       wip = awinc;
       for (i = 0;i<nn;i++,cp++,wp++,wip++) {
           ci = *cp;
           w = *wp;
           wi = *wip;
           printf("#%i  ",i+1);
           for(j=0;j<(nn+ni+1);j++,ci++,w++,wi++) {
               if(*wi<0.0) printf("%f ",*wi);
               else printf(" %f ",*wi);
           }
           printf("\n");
       }
       */
    /* Under bptt, the weight increments now need to be summed
       across copies. They can then be applied to their
       respective weights. */
    if(bptt) {                  /* new bptt code... */
        /* First, sum wi's across bias copies. */
        cp = cinfo;
        wip = awinc;
        wp = awt;
        for (i = 0; i < true_nn; i++,cp++,wip++,wp++) {
            ci = *cp;
            if(ci->con) {
                wi_sum = 0.0;
                for (j = 0; j < copies; j++) {
                    wi_sum += **(wip+(j*true_nn));
                }
                for (j = 0; j < copies; j++) {
                    **(wp+(j*true_nn)) += wi_sum;
                }
            }
        }
        /* Now, sum wi's across inputs. */
        cp = cinfo;
        wip = awinc;
        wp = awt;
        for (i = 0; i < true_nn; i++,cp++,wip++,wp++) {
            for (j = 1; j <= true_ni; j++) {
                ci = *cp + j;
                if(ci->con) {
                    wi_sum = 0.0;
                    for (k = 0; k < copies; k++) {
                        wi_sum += *(*(wip+(k*true_nn))+j+(k*true_ni));
                    }
                    for (k = 0; k < copies; k++) {
                        *(*(wp+(k*true_nn))+j+(k*true_ni)) += wi_sum;
                    }
                }
            }
        }
        /* Next, we do the feed forward's... */
        cp = cinfo;
        wip = awinc;
        wp = awt;
        for (i = 0; i < true_nn; i++,cp++,wip++,wp++) {
            for (j = 0; j < true_nn; j++) {
                ci = *cp + j + 1 + ni;
                /* Note the absense of a check for a
                   connection - this is because optimization
                   of the network cut off the feed forward
                   connections on the bottome of the copy stack,
                   so we can't rely on early ci->con's as
                   indicative of later copy ci->con's. */
                wi_sum = 0.0;
                for (k = 0; k < copies; k++) {
                    wi_sum += *(*(wip+(k*true_nn))+j+ni+1+(k*true_nn));
                }
                for (k = 0; k < copies; k++) {
                    *(*(wp+(k*true_nn))+j+ni+1+(k*true_nn)) += wi_sum;
                }
            }
        }
        /* Finally, we do the recurrent's... */
        cp = cinfo;
        wip = awinc;
        wp = awt;
        for (i = 0; i < true_nn; i++,cp++,wip++,wp++) {
            for (j = 0; j < true_nn; j++) {
                ci = *(cp+true_nn) + j + 1 + ni;
                if(ci->con) {
                    wi_sum = 0.0;
                    for (k = 0; k < (copies-1); k++) {
                        wi_sum += *(*(wip+(k*true_nn)+true_nn)+j+ni+1+(k*true_nn));
                    }
                    for(k = 0; k < (copies-1); k++) {
                        *(*(wp+(k*true_nn)+true_nn)+j+ni+1+(k*true_nn)) += wi_sum;
                    }
                }
            }
        }
    }
    /* DEBUG */
    /*
       printf("Summed weight increment matrix:\n");
       cp = cinfo;
       wp = awt;
       wip = awinc;
       for (i = 0;i<nn;i++,cp++,wp++,wip++) {
           ci = *cp;
           w = *wp;
           wi = *wip;
           printf("#%i  ",i+1);
           for(j=0;j<(nn+ni+1);j++,ci++,w++,wi++) {
               if(*wi<0.0) printf("%f ",*wi);
               else printf(" %f ",*wi);
           }
           printf("\n");
       }
       */
    /* DEBUG */
    /*
       printf("Incremented weight matrix:\n");
       cp = cinfo;
       wp = awt;
       wip = awinc;
       for (i = 0;i<nn;i++,cp++,wp++,wip++) {
           ci = *cp;
           w = *wp;
           wi = *wip;
           printf("#%i  ",i+1);
           for(j=0;j<(nn+ni+1);j++,ci++,w++,wi++) {
               if(*w<0.0) printf("%f ",*w);
               else printf(" %f ",*w);
           }
           printf("\n");
       }
       */
    /* ********************************** */



    /* look for weights in the same group and average them together */
    for (k = 1; k <= ngroups; k++){
        *sum = 0.;
        n = 0;
        cp = cinfo;
        wp = awt;
        /* calculate average */
        for (i = 0; i < nn; i++, cp++, wp++){
            ci = *cp;
            w = *wp;
            for (j = 0; j < nt; j++, w++, ci++){
                if (ci->num == k){
                    *sum += *w;
                    n++;
                }
            }
        }
        if (n > 0)
            *sum /= n;
        /* replace weight with average */
        cp = cinfo;
        wp = awt;
        for (i = 0; i < nn; i++, cp++, wp++){
            ci = *cp;
            w = *wp;
            for (j = 0; j < nt; j++, w++, ci++){
                if (ci->num == k)
                    *w = *sum;
            }
        }
    }
    /* look for limited weights and enforce limits */

    if (limits == 0)
        return NO_ERROR;

    /* DEBUG LIMITS */
    /*
       printf("Looking for weight limits...\n");
    */

    cp = cinfo;
    wp = awt;
    for (i = 0; i < nn; i++, cp++, wp++){
        ci = *cp;
        w = *wp;
        for (j = 0; j < nt; j++, w++, ci++){
            if (ci->lim){
                /* DEBUG LIMITS */
                /*
                   printf("Limiting Node %i     Connection %j   ",i,j);
                */
                if (*w < ci->min)
                    *w = ci->min;
                /*
                   printf("min = %f     max = %f\n",ci->min, ci->max);
                */
                if (*w > ci->max)
                    *w = ci->max;
            }
        }
    }
    return NO_ERROR;
}


