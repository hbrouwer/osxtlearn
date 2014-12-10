#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "error.h"
#include "parse.h"


extern  int       nn;             /* number of nodes */
extern  int       ni;             /* number of inputs */
extern  int       no;             /* number of outputs */
extern  int       nt;             /* nn + ni + 1 */
extern  int       np;             /* ni + 1 */

extern  int       true_no;        /* number of outputs in original network. */
extern  CFStruct  **cinfo;        /* (nn x nt) connection info */
extern  NFStruct  *ninfo;         /* (nn) node info */

extern  FILE      *cfp;

extern  int       *outputs;       /* (no) indices of output nodes */
extern  int       *selects;       /* (nn+1) nodes selected for probe printout */

extern  int       ngroups;        /* number of groups */
extern  int       limits;         /* flag for limited weights */

extern  float     weight_limit;   /* bound for random weight init */



/*NEW:*/
/* For architecture displays: */
extern int make_new_layer();
extern int make_new_slab();
extern int prepare_architecture_memory();


/* **************************************** */
extern int bptt;        /* Our flag. */
extern int copies;      /* Number of steps to unfold. */
extern int true_nn;     /* Node count variables (see tlearn.c) */
extern int true_ni;

/* ******************** */

static char nbuf[80];   /* shows string in get_nums */
static int nbp;

static int get_str(FILE *fp, char *buf, char *str);


int get_nodes(void)
{
    int i;

    char        buf[80];
    char        tmp[80];

    /* read nn, ni, no */
    nn = ni = no = -1;
    if (get_str(cfp,buf,"\n")) return ERROR;
    /* first line must be "NODES:" */
    if (strcmp(buf, "NODES:") != 0)
        return parse_err("First line must be \"NODES:\".");

    /* next three lines must specify nn, ni, and no in any order */
    for (i = 0; i < 3; i++){
        if (get_str(cfp,buf," ")) return ERROR;
        if (get_str(cfp,tmp," ")) return ERROR;
        if (tmp[0] != '=' || tmp[1] != 0)
            return parse_err("Line should be: \"xxxxx = n\".  xxxxx is 'nodes', 'inputs' or 'outputs'.  Spaces around '=' are required.");
        if (get_str(cfp,tmp,"\n")) return ERROR;
        if (strcmp(buf, "nodes") == 0)
            nn = atoi(tmp);
        else if (strcmp(buf, "inputs") == 0)
            ni = atoi(tmp);
        else if (strcmp(buf, "outputs") == 0)
            no = atoi(tmp);
        else return parse_err("Line should be: \"xxxxx = n\".  xxxxx is 'nodes', 'inputs' or 'outputs'.");
    }
    if ((nn < 1) || (ni < 0) || (no < 0) || (nn < no))
        return parse_err("Invalid number of input, hidden, or output nodes in configuration file.");

    nt = 1 + ni + nn;
    np = 1 + ni;
        
    return NO_ERROR;
}

int get_outputs(void)
{
    int i;
    int j;

    NFStruct    *n;

    char        buf[80];

    if (get_str(cfp,buf," ")) return ERROR;
    /* next line must specify outputs */
    if (strcmp(buf, "output") != 0)
        return parse_err("Line should specify outputs nodes.");
    if (get_str(cfp,buf," ")) return ERROR;
    if (strcmp(buf, "node") != 0){
        if (strcmp(buf, "nodes") != 0)
            return parse_err("Line should be: \"output node ...\" or \"output nodes ...\"");
    }
    if (get_str(cfp,buf," ")) return ERROR;
    if (strcmp(buf, "are") != 0){
        if (strcmp(buf, "is") != 0)
            return parse_err("Line should be: \"output node is ...\" or \"output nodes are ...\"");
    }
    if (get_str(cfp,buf,"\n")) return ERROR;
    if (get_nums(buf,nn+ni,ni,selects)) return ERROR;


    /* **************************************************** */
    /* The outputs of the new, feed forward
       network are the same in number as
       in the original recursive network,
       but are actually found not at
       location i, but at location
       i+(copies-1)*(true_nn+true_ni).
       In other words, the output is pinned
       to the last copy. */
    /* Disregard last note; outputs are now
       windowed, which means that we want
       the ouputs duplicated across copies. */

    if (bptt) {
        for (i=ni+1; i<=true_nn + ni; i++) {
            if (selects[i] == 1) {
                for( j = 1; j< copies; j++) {
                    selects[i+(j*true_nn)] = 1;
                }
            }
        }
    }
    /* ******************************** */
    /* DEBUG*/
    /* show output nodes */
    /*
       printf("Output nodes:\n");
       for(i=1;i<=nn+ni;i++) {
           if(selects[i]==1) {
               printf("output Node %i\n",i-ni);
           }
       }
       printf("-------------------\n");
       */

    for (i = 1; i <= ni; i++){
        if (selects[i] == 1)
            return parse_err("An input cannot be an output.");

        /* **************************************************** */
        if (bptt) {
            for (j=1; j<ni; j++) {
                if (selects[j] == 1)
                    return parse_err("An input cannot be an output.");
            }
        }
        /* ******************************** */
    }




    if (selects[0] == 1)
        return parse_err("Node zero (the bias node) cannot be an output.");

    n = ninfo;
    for (i = ni+1, j = -1; i <= nn+ni; i++, n++){
        if (selects[i] > 0){
            if (++j < no){
                outputs[j] = i-ni;
                n->targ = 1;
            }
        }
    }
    if (++j != no)
        return parse_err("Discrepant number of outputs.");

    return NO_ERROR;
}

/* These two pointers need to be freed before get_connections returns */
/* They are freed only by the function gc_return();                        */

static int gc_return(int return_value);
static int *gc_tmp;
static int *gc_iselects;

static int gc_return(int return_value)
{
    if (gc_tmp) free(gc_tmp);
    if (gc_iselects) free(gc_iselects);
    gc_tmp= NULL;
    gc_iselects= NULL;
    
    return return_value;
}

int get_connections(void)
{
    int i;
    int j;
    int k;
    int offset;
    int last_rec_node;
                        /* Any node beyond the "last recurrent node" won't contribute
                           in the first (copies-1) copies in the feed-forward network;
                           this is used to mark such a node, and we'll disable all
                           connections made after it. (used in get_connections) */

    CFStruct    *ci;
    /* ***************************** */
    CFStruct    *copy_ci;       /* Used for bptt matrix transformation. */
    /* ***************************** */

    int gn;

    float       min;
    float       max;

    char        buf[80];

    /* calloc space for gc_iselects */
    gc_iselects = (int *) calloc(nt, sizeof(int));
    if (gc_iselects == NULL) return gc_return(memerr("gc_iselects"));

    if (get_str(cfp,buf,"\n")) return gc_return(ERROR);
    /* next line must be "CONNECTIONS:" */
    if (strcmp(buf, "CONNECTIONS:") != 0)
        return gc_return(parse_err("Line should be: \"CONNECTIONS:\"."));

    if (get_str(cfp,buf," ")) return gc_return(ERROR);

    /* next line must be "groups = #" */
    if (strcmp(buf, "groups") != 0)
        return gc_return(parse_err("Line should specify number of groups."));
    if (get_str(cfp,buf," ")) return gc_return(ERROR);
    if (buf[0] != '=' || buf[1])
        return gc_return(parse_err("Line should be: \"groups = ...\".  Spaces around '=' are required." ));
    if (get_str(cfp,buf,"\n")) return gc_return(ERROR);
    ngroups = atoi(buf);

    /* calloc space for gc_tmp */
    gc_tmp = (int *) calloc((ngroups+1), sizeof(int));
    if (gc_tmp == NULL) return gc_return(memerr("gc_tmp"));

    if (get_str(cfp,buf," ")) return gc_return(ERROR);

    limits= 0;  /* default: no limits */

    while (strcmp(buf, "SPECIAL:") != 0){
        /* a group is identified */
        if (strcmp(buf,"group") == 0){
            if (get_str(cfp,buf," ")) return gc_return(ERROR);
            if (get_nums(buf,ngroups,0,gc_tmp)) return gc_return(ERROR);
            if (get_str(cfp,buf," ")) return gc_return(ERROR);
            if (buf[0] != '=' || buf[1])
                return gc_return(parse_err("Expecting '=' with spaces around it."));
            if (get_str(cfp,buf," ")) return gc_return(ERROR);
            /* group * = fixed */
            if (strcmp(buf,"fixed") == 0){
                for (i = 0; i < nn; i++){
                    ci = *(cinfo + i);
                    for (j = 0; j < nt; j++, ci++){
                        if (gc_tmp[ci->num])
                            ci->fix = 1;
                    }
                }
            }
            /* group * = wmin & wmax */
            else {
                min = (float) atof(buf);
                if (get_str(cfp,buf," ")) return gc_return(ERROR);
                if (buf[0] != '&' || buf[1])
                    return gc_return(parse_err("Expecting '&' with spaces around it."));
                if (get_str(cfp,buf," ")) return gc_return(ERROR);
                max = (float) atof(buf);
                if (max < min)
                    return gc_return(parse_err("Group max value < min value."));

                for (i = 0; i < nn; i++){
                    ci = *(cinfo + i);
                    for (j = 0; j < nt; j++, ci++){
                        if (gc_tmp[ci->num]){
                            limits = 1;
                            ci->lim = 1;
                            ci->min = min;
                            ci->max = max;
                        }
                    }
                }
            }

            if (get_str(cfp,buf," ")) return gc_return(ERROR);
        }
        /* a connection is specified */
        else {
            if (get_nums(buf,nn+ni,ni,selects)) return gc_return(ERROR);
            if (selects[0])
                return gc_return(parse_err("Connecting TO node zero (the bias node) is not legal."));

            for (i = 1; i <= ni; i++){
                if (selects[i])
                    return gc_return(parse_err("Connecting TO an input is not legal."));
            }

            if (get_str(cfp,buf," ")) return gc_return(ERROR);
            if (strcmp(buf,"from") != 0)
                return gc_return(parse_err("Expecting \"from ...\"."));
            if (get_str(cfp,buf," ")) return gc_return(ERROR);
            if (get_nums(buf,nn+ni,ni,gc_iselects)) return gc_return(ERROR);
            for (i = 0; i < nn; i++){
                ci = *(cinfo + i);
                for (j = 0; j < nt; j++, ci++){
                    if ((selects[i+ni+1]) && (gc_iselects[j]))
                        ci->con += 1;
                }
            }

            if (get_str(cfp,buf," ")) return gc_return(ERROR);
            if (buf[0] == '=') {
                if (buf[1]) return gc_return(parse_err("Expecting '=' with spaces around it."));
                if (get_str(cfp,buf," ")) return gc_return(ERROR);
                /* connection  = fixed */
                if (strcmp(buf,"fixed") == 0){
                    for (i = 0; i < nn; i++){
                        ci = *(cinfo + i);
                        for (j = 0; j < nt; j++, ci++){
                            if ((selects[i+ni+1]) &&
                                (gc_iselects[j]))
                                ci->fix = 1;
                        }
                    }
                }
                /* Used to force recurrent connection... */
                /* ********************************** */
                else if (strcmp(buf,"recurrent") == 0){
                    /* selects[] is "to" nodes, gc_iselects[]
                       is "from nodes. */
                    /* We'll just re-use the following
                       group code to establish forced
                       recurrence. */
                    for (i = 0; i < nn; i++){
                        ci = *(cinfo + i);
                        for (j = 0; j < nt; j++, ci++){
                            if ((selects[i+ni+1]) &&
                                (gc_iselects[j]))
                                ci->rec = 1;
                        }
                    }
                }
                /* ********************************** */
                else {
                    /* connection  = group # */
                    if (strcmp(buf,"group") == 0){
                        if (get_str(cfp,buf," ")) return gc_return(ERROR);
                        gn = atoi(buf);
                        for (i = 0; i < nn; i++){
                            ci = *(cinfo + i);
                            for (j = 0; j < nt; j++, ci++){
                                if ((selects[i+ni+1]) &&
                                    (gc_iselects[j]))
                                    ci->num = gn;
                            }
                        }
                    }
                    /* connection  = min & max */
                    else {
                        min = (float) atof(buf);
                        if (get_str(cfp,buf," ")) return gc_return(ERROR);
                        if (buf[0] != '&' || buf[1])
                            return gc_return(parse_err("Expecting '&' with spaces around it."));
                        if (get_str(cfp,buf," ")) return gc_return(ERROR);
                        max = (float) atof(buf);
                        if (max < min)
                            return gc_return(parse_err("Connection max value < min value."));

                        for (i = 0; i < nn; i++){
                            ci = *(cinfo + i);
                            for (j = 0; j < nt; j++, ci++){
                                if ((selects[i+ni+1]) &&
                                    (gc_iselects[j])){
                                    limits = 1;
                                    ci->lim = 1;
                                    ci->min = min;
                                    ci->max = max;
                                }
                            }
                        }
                    }
                }
                if (get_str(cfp,buf,"\t")) return gc_return(ERROR);
                if (strcmp(buf,"fixed") == 0){
                    for (i = 0; i < nn; i++){
                        ci = *(cinfo + i);
                        for (j = 0; j < nt; j++, ci++){
                            if ((selects[i+ni+1]) &&
                                (gc_iselects[j]))
                                ci->fix = 1;
                        }
                    }
                    if (get_str(cfp,buf,"\t")) return gc_return(ERROR);
                }
                if (strcmp(buf,"one-to-one") == 0){
                    for (k = 0; k < nt; k++){
                        if (gc_iselects[k])
                            break;
                    }
                    for (i = 0; i < nn; i++){
                        ci = *(cinfo + i);
                        for (j = 0; j < nt; j++, ci++){
                            if ((selects[i+ni+1]) &&
                                (gc_iselects[j])){
                                if (ci->con == 1){
                                    ci->con = 0;
                                    ci->fix = 0;
                                    ci->lim = 0;
                                }
                                else
                                    ci->con -= 1;
                            }
                        }
                        if (selects[i+np]){
                            ci = *(cinfo + i) + k++;
                            ci->con = 1;
                            ci->fix = 1;
                            ci->lim = 1;
                        }
                    }
                    if (get_str(cfp,buf,"\n")) return gc_return(ERROR);
                }
            }
        }
    }


    if (bptt) {
        /* Tricky stuff here. We've got to modify the connections
           matrix so that all forward connections remain intact,
           all recurrent connections are forward connected
           to their respective unit copies, and that all input
           connections are duplicated correctly. Effectively, if
           a connection is marked recurrent in the .cf file,
           it is moved down one copy. For instance, if we
           originally mark connection 2 from 1 as recurrent,
           it will be moved to the next copy so that the matrix
           will have 2+nn from 1 as a connection, not 2 from 1.
           For the inputs, we will duplicate them (copies times) in
           the matrix down true_nn cells and to the right true_ni
           cells. In other words, each copy of the original network
           has its own set of inputs.
           It may help to draw the 2-D connection matrix to see
           what is happening. 
           IMPORTANT: ci->rec is multi-valued;
           a 0 indicates no recurrent connection, a 1 indicates a
           recurrent connection at that node-node position in the
           connection matrix, and a 2 indicates a recurrent connection
           which has been MOVED. So, really, in the folded up network, 
           the connections will really be found at the 2's positions. */

        /* First, duplicate the bias connections for each
           copy. */

        for (i = 0; i < true_nn; i++) {
            ci = *(cinfo + i);
            for (j = 1; j < copies; j++) { /* This is our copy target */
                copy_ci = *(cinfo + i + (j*true_nn));
                /* ie, one duplication of each bias connection
                   for each copy. */
                copy_ci->con = ci->con;
                copy_ci->fix = ci->fix;
                copy_ci->num = ci->num;
                copy_ci->lim = ci->lim;
                copy_ci->min = ci->min;
                copy_ci->max = ci->max;
            }
        }

        /* Now, we duplicate the inputs across copies. Since
           the inputs are unique for each copy, we not only
           duplicate them down the connection matrix, but
           shift each one to the right enough to make the
           connections unique. */

        for (i = 0; i < true_nn; i++) {
            ci = *(cinfo + i);
            for (j = 1; j <= true_ni ; j++) {
                ci++;
                /* get each node's input matrix. */
                for (k = 1; k < copies; k++) {
                    /* get to correct copy... */
                    copy_ci = *(cinfo + i + (k*true_nn));
                    /* get to correct input connection... */
                    copy_ci = copy_ci + j;
                    /* offset copy's matrix... */
                    copy_ci = copy_ci + (k * true_ni);

                    copy_ci->con = ci->con;
                    copy_ci->fix = ci->fix;
                    copy_ci->num = ci->num;
                    copy_ci->lim = ci->lim;
                    copy_ci->min = ci->min;
                    copy_ci->max = ci->max;
                }
            }
        }
        /* Note: The way this is now structured, the first
           set of inputs (n1..true_ni) is connected to the
           first copy, which is the most distant time step
           of the network. Therefore, inputs being read
           from the data file should be placed on a queue
           (as the most recent inputs), and this queue
           should be placed across all of the inputs. This
           way, the most recent input is placed on the
           _last_ copy, so its influence is strongest on
           the output, since the output is found in the last
           copy. */

        /* Next, we disconnect all recurrent connections from
           our first copy (which are copy-internal) and reconnect
           them to the next level (thereby making them
           feed-forward connections. Since each copy will
           be identical to this one (ie, copies one and two
           and their interconnections), we'll then duplicate all
           of copy one to each other copy, dumping the
           level-feed-forward connections for the last copy.
           Make sense? The only connections shifted to the
           next copy are those explicitly marked recurrent. */
                
        last_rec_node = -1;
        for (i = 0; i < true_nn; i++) {
            offset = ni + 1;
            ci = *(cinfo + i);
            ci = ci + (offset);
            copy_ci = *(cinfo + i + true_nn);
            copy_ci = copy_ci + offset;
            for (j = 0; j < true_nn; j++) {
                /* Only shift connection to next copy
                   if connection is marked recurrent. */
                if( ci->rec ) {

                    copy_ci->con = ci->con;
                    copy_ci->fix = ci->fix;
                    copy_ci->num = ci->num;
                    copy_ci->lim = ci->lim;
                    copy_ci->min = ci->min;
                    copy_ci->max = ci->max;
                    ci->rec = 2;
                    copy_ci->rec = 0;
                    /* NOTE: For an unfolded bptt network, connections marked 2
                       in the recurrent field are not recurrent, but they signify that
                       they represent a recurrent connection in the folded network. In
                       other words, connections which have been moved for bptt. */
                    last_rec_node = j; /* Remember where our */
                    /* last needed conection */
                    /* is. */
                    ci->con = 0; /* Erase any recurrent connection. */
                }
                ci++;
                copy_ci++;
            }
        }

        if (last_rec_node == -1) 
            report_condition("<bptt warning> No recurrent connections were found.", 1);

        /* Now duplicate this network for each copy. If we're
           duplicating to the last copy, don't copy the
           forward connections (since there's no next
           level); instead, just zero them out. end_node
           checks for this condition. */

        for (i = 1; i < copies; i++) {
            int end_node;
            if (i==(copies-1)) {
                end_node = true_nn;
            }
            else {
                end_node = 2 * true_nn;
            }
            for (j = 0; j < end_node; j++) {
                ci = *(cinfo + j);
                ci = ci + ni + 1;
                copy_ci = *(cinfo + j + (i*true_nn));
                copy_ci = copy_ci + (i*true_nn) + ni + 1;
                for (k = 0; k < true_nn; k++) {
                    copy_ci->con = ci->con;
                    copy_ci->fix = ci->fix;
                    copy_ci->num = ci->num;
                    copy_ci->lim = ci->lim;
                    copy_ci->min = ci->min;
                    copy_ci->max = ci->max;
                    copy_ci->rec = ci->rec;

                    ci++;
                    copy_ci++;
                }
            }
        }
    } /* end check for bptt flag */

    /* ************************** */




    /*
       for (i = 0; i < nn; i++){
       ci = *(cinfo + i);
       for (j = 0; j < nt; j++, ci++){
       fprintf(stderr,"i: %d  j: %d  c: %d  f: %d  g: %d\n",
       i,j,ci->con,ci->fix,ci->num);
       }
       }
       */

    return gc_return(NO_ERROR);
}


int get_special(void)
{
    char        buf[80];
    char        tmp[80];

    int i, err= 0;

    int *iselects;

    NFStruct    *n;

    /* calloc space for iselects */
    iselects = (int *) calloc(nt, sizeof(int));
    if (iselects == NULL) return memerr("iselects");

    /* NEW: Here, we prepare the architecture display
       structure so we can tell it where layers and
       slabs are. */
    if (prepare_architecture_memory(nt)) return -1;

    weight_limit= 1.0;  /* default weight_limit */

    while (fscanf(cfp,"%s",buf) != EOF) {
        err= -1;     /* any break from this loop is an error */
        if (get_str(cfp,tmp," ")) break;
        if (tmp[0] != '=' || tmp[1]) {
            parse_err("Expecting '=' with spaces around it.");
            break;
        }
        if (get_str(cfp,tmp,"\n")) break;
        if (strcmp(buf,"weight_limit") == 0)
            weight_limit = (float) atof(tmp);
        if (strcmp(buf,"selected") == 0){
            if (get_nums(tmp,nn,0,selects)) break;
        }
        if (strcmp(buf,"linear") == 0){
            if (get_nums(tmp,nn,0,iselects)) break;
            n = ninfo;
            for (i = 1; i <= nn; i++, n++){
                if (iselects[i])
                    n->func = 2;
            }
        }
        if (strcmp(buf,"bipolar") == 0){
            if (get_nums(tmp,nn,0,iselects)) break;
            n = ninfo;
            for (i = 1; i <= nn; i++, n++){
                if (iselects[i])
                    n->func = 1;
            }
        }
        if (strcmp(buf,"binary") == 0){
            if (get_nums(tmp,nn,0,iselects)) break;
            n = ninfo;
            for (i = 1; i <= nn; i++, n++){
                if (iselects[i])
                    n->func = 3;
            }
        }
        if (strcmp(buf,"delayed") == 0){
            if (get_nums(tmp,nn,0,iselects)) break;
            n = ninfo;
            for (i = 1; i <= nn; i++, n++){
                if (iselects[i])
                    n->dela = 1;
            }
        }
        /*DEBUG New ideas for dealing with layers:
         */
        /*NEW:*/
        if (strcmp(buf,"layer") == 0) {
            if (get_nums(tmp,nn+ni,ni,iselects)) break;
            if (make_new_layer(iselects)) break;
        }
        if (strcmp(buf,"slab") == 0) {
            if (get_nums(tmp,nn+ni,ni,iselects)) break;
            if (make_new_slab(iselects)) break;
        }

    err= 0;   /* made it through the loop error free */
    }
    
    if (iselects) free(iselects);
    iselects = NULL;
    return err;
}


static int get_str(FILE *fp, char *buf, char *str)
{
    if (fscanf(fp,"%s",buf) == EOF)
        return parse_err("Unexpected end of file.");

    return NO_ERROR;
}


int get_nums(char *str, int nv, int offset, int *vec)
{
        int     c, i, j, l, k, m= 0, n;
        int     dash= 0;
        int     input= 0;

        char    tmp[80];

        dash = 0;
        input = 0;
        l = strlen(str);
        nbp = 0;

        for (i = 0; i <= nv; i++)
                vec[i] = 0;
        for (i = 0, j = 0; i < l; j++, i++){
                c = str[i];
                switch (c) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':       nbuf[nbp++] = tmp[j] = str[i];
                                        break;
                        case 'i':       input++;
                                        j--;
                                        nbuf[nbp++] = str[i];
                                        break;
                        case '-':       if (j == 0 || i == l -1)
                                                return parse_err("Dash (-) is misplaced.");
                                        tmp[j] = '\0';
                                        j = -1;
                                        nbuf[nbp++] = str[i];
                                        m = atoi(tmp);
                                        dash = 1;
                                        break;
                        case ',':       if (j == 0)
                                                return parse_err("Comma is misplaced.");
                                        tmp[j] = '\0';
                                        j = -1;
                                        nbuf[nbp++] = str[i];
                                        if (dash){
                                                n = atoi(tmp);
                                                if (input == 1)
                                                        return parse_err("Comma is misplaced.");
                                                if (n < m)
                                                        return parse_err("Comma is misplaced.");
                                        }
                                        else{
                                                m = atoi(tmp);
                                                n = m;
                                        }
                                        if (input == 0){
                                                m += offset;
                                                n += offset;
                                        }
                                        if (n > nv){
                                                sprintf(tmp, "There is no node number %d. Node numbers should not exceed %d.", n -offset, nv -offset);
                                                return parse_err(tmp);
                                        }
                                        if ((input) && (n > offset)){
                                                sprintf(tmp, "There is no input number %d. Input numbers should not exceed %d.", n, offset);
                                                return parse_err(tmp);
                                        }
                                        for (k = m; k <= n; k++){
                                                if ((input == 0) && (k == offset))
                                                        vec[0] = 1;
                                                else
                                                        vec[k] = 1;
                                        }
                                        input = 0;
                                        dash = 0;
                                        break;
                                        
                        default:        return parse_err("Unable to parse line.");
                }
        }
        if (j == 0) return parse_err("Unable to parse line.");

        tmp[j] = '\0';
        nbuf[nbp++] = tmp[j];
        if (dash){
                n = atoi(tmp);
                if (input == 1)
                        return parse_err("Cannot use dash to connect input and non-input.");

                if (n < m)
                        return parse_err("Upper bound must exceed lower bound.");

        }
        else{
                m = atoi(tmp);
                n = m;
        }
        if (input == 0){
                m += offset;
                n += offset;
        }
        if (n > nv){
                sprintf(tmp, "There is no node number %d. Node numbers should not exceed %d.", n -offset, nv -offset);
                return parse_err(tmp);
        }
        if ((input) && (n > offset)){
                sprintf(tmp, "Configuration file: There is no input number %d. Input numbers should not exceed %d.", n, offset);
                return parse_err(tmp);
        }
        for (k = m; k <= n; k++){
                if ((input == 0) && (k == offset))
                        vec[0] = 1;
                else
                        vec[k] = 1;
        }

        nbp = 0;

        return NO_ERROR;
}

