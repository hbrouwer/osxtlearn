#include <math.h>
#include <stdlib.h>
#include "general.h"
#include "error.h"
#include "compute.h"

extern  int     nn;             /* number of nodes */
extern  int     ni;             /* number of inputs */
extern  int     no;             /* number of outputs */
extern  int     nt;             /* nn + ni + 1 */
extern  int     np;             /* ni + 1 */
extern  int     ce;             /* cross-entropy flag */

extern  CFStruct        **cinfo;        /* (nn x nt) connection info */
extern  NFStruct        *ninfo;         /* (nn) node activation function info */

extern  int     *outputs;       /* (no) indices of output nodes */

extern  int     localist;       /* flag for localist input */

static  float   *terror = 0;
static  float   *ce_terror = 0;


int init_compute(void)
{
    if (terror) free(terror);
    terror= NULL;              /* calloc space for local copy of error info */
    terror= (float *) calloc(no, sizeof(float));
    if (terror == NULL) return memerr("terror");

    if (ce_terror) free(ce_terror);
    ce_terror= NULL;   /* calloc space for local copy of cross-entropy info */
    ce_terror= (float *) calloc(no, sizeof(float));
    if (ce_terror == NULL) return memerr("ce_terror");

    return 0;
}



int comp_errors(float *aold, float *atarget, float *aerror, float *e, float *ce_e)
{
    extern      int ce;

    register    int     i;
    register    int     j;
    register    float   *ta;
    register    float   *te;
    register    float   *ce_te;
    register    float   *ee;
    register    int     *op;


    te = terror;
    ce_te = ce_terror;
    ta = atarget;
    op = outputs;
    for (i = 0; i < no; i++, te++, ce_te++, ta++, op++){
        if (*ta != -9999.0) {
            *te = *(aold + ni + *op) - *ta;
            /*
             * if collecting cross-entropy statistics;
             */
            if (ce == 2) {
                *ce_te = *ta * log(*(aold+ni+ *op))/log(2.0) +
                    (1- *ta) * log(1- *(aold+ni+ *op))/log(2.0);
            }
        } else {
            *te = 0.;
        }
        *e += *te * *te;        /* cumulative ss error */
        *ce_e += *ce_te;        /* cumulate cross-entropy error */
    }
    ee = aerror;
    for (i = 1; i <= nn; i++, ee++){
        *ee = 0.;
        te = terror;
        op = outputs;
        for (j = 0; j < no; j++, te++, op++){
            if (*op == i){
                *ee = *te;
                break;
            }
        }
    }
    return 0;
}


int comp_backprop(float **awt, float **adwt, float *aold, 
                  float *amem, float *atarget, float *aerror, int *local)
{
    register    int     i;
    register    int     j;

    register    CFStruct        **cp;

    register    CFStruct        *ci;
    register    NFStruct        *n;

    register    float   *sum;

    float       **wp;
    float       *ee;
    float       *e;
    float       *w;
    float       *z;
    float       *oz;
    float       *t;

    int *l;
    int ns;

    float       asum;

    /* compute deltas for output units */
    sum = &asum;
    e = aerror;
    n = ninfo;
    z = aold + np;
    t = atarget;
    for (i = 0; i < nn; i++, e++, n++, z++){
        if (n->targ == 0)
            continue;
        if (n->func == 0) {
            if (ce > 0) {       /* if cross-entropy */
                /*
                 * note that the following collapses 
                 * (t-a) and derivative of slope; we
                 * therefore ignore current contents of
                 * *e (which is (t-a)) and assign new
                 * value, whereas with sse, we multiply *e
                 * by deriv. of slope.
                 */
                *e = *t - *z;
                /* NOTE: this is a kludge -- only increments
                 * target when node is an output node. Do
                 * NOT move into for() control statement.
                 */
                t++;
            } else {            /* otherwise normal sse-delta */
                *e *= *z * (1. - *z);
            }
        } else if (n->func == 1)
            *e *= .5 * (1. + *z) * (1. - *z);
    }

    n = ninfo + nn - 1;
    z = aold + nt - 1;
    e = aerror + nn - 1;
    /* compute deltas for remaining units */
    for (i = nn - 1; i >= 0; i--, z--, e--, n--){
        if (n->targ == 1)
            continue;
        *sum = 0.;
        /* ee contains a bad address for i = nn-1 */
        ee = aerror + i + 1;
        for (j = i + 1; j < nn; j++, ee++){
            w = *(awt + j) + np + i;
            ci = *(cinfo + j) + np + i;
            if (ci->con)
                *sum += *w * *ee;
        }
        if (n->func == 0)
            *e = *sum * *z * (1. - *z);
        else if (n->func == 1)
            *e = *sum * .5 * (1. + *z) * (1. - *z);
        else if (n->func == 2){
            *e = *sum;
        }
        else if (n->func == 3)
            *e = 0.;
    }

    /* compute weight changes for all connections */

    /* to each node */
    e = aerror;
    cp = cinfo;
    wp = adwt;
    for (i = 0; i < nn; i++, e++, cp++, wp++){
        if (localist){
            if (ce > 0){
                if ((*cp)->con)
                    **wp += *e;
            }
            else {
                if ((*cp)->con)
                    **wp -= *e;
            }
            l = local;
            while (*l != 0){
                if (ce > 0){
                    if ((*cp + *l)->con)
                        *(*wp + *l) += *e;
                }
                else {
                    if ((*cp + *l)->con)
                        *(*wp + *l) -= *e;
                }
                l++;
            }
            w = *wp + np;
            ci = *cp + np;
            z = aold + np;
            oz = amem + np;
            /* from each node */ 
            /* loop is broken into two parts:
               (1) connections from nodes of lower node-number
               (2) connections from nodes of = or > node-number
               the latter case requires use of old z values */
            if (ce > 0){
                for (j = 0; j < i; j++, w++, ci++, z++, oz++){
                    if (ci->con)
                        *w += *z * *e;
                }
                for (j = i; j < nn; j++, w++, ci++, z++, oz++){
                    if (ci->con)
                        *w += *oz * *e;
                }
            }
            else {
                for (j = 0; j < i; j++, w++, ci++, z++, oz++){
                    if (ci->con)
                        *w -= *z * *e;
                }
                for (j = i; j < nn; j++, w++, ci++, z++, oz++){
                    if (ci->con)
                        *w -= *oz * *e;
                }
            }
        }
        else {
            w = *wp;
            ci = *cp;
            z = aold;
            oz = amem;
            /* from each bias, input, and node */ 
            ns = np + i;
            /* loop is broken into two parts:
               (1) connections from nodes of lower node-number
               (2) connections from nodes of = or > node-number
               ** the latter case requires use of old z values */
            /* NOTE the use of ci->rec for recurrent
               connections. */
            if (ce > 0){
                for (j = 0; j < ns; j++, w++, ci++, z++, oz++){
                    if (ci->con) {
                        if(ci->rec==1) *w += *oz * *e;
                        else *w += *z * *e;
                    }
                }
                for (j = ns; j < nt; j++, w++, ci++, z++, oz++){
                    if (ci->con)
                        *w += *oz * *e;
                }
            }
            else {
                for (j = 0; j < ns; j++, w++, ci++, z++, oz++){
                    if (ci->con) {
                        if(ci->rec==1) *w -= *oz * *e;
                        else *w -= *z * *e;
                    }
                }
                for (j = ns; j < nt; j++, w++, ci++, z++, oz++){
                    if (ci->con)
                        *w -= *oz * *e;
                }
            }
        }
    }

    return 0;
}

