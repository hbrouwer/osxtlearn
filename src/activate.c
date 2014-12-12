#include <math.h>
#include "general.h"
#include "error.h"
#include "activate.h"


#ifdef EXP_TABLE
extern  float   *exp_array;     /* table look-up for exp function */
extern  float   exp_mult;
extern  long    exp_add;
#define EXP(m) (exp_array[((int) ((m) * exp_mult)) + exp_add])
#else
#define EXP exp
#endif /* EXP_TABLE */


extern  int nn;             /* number of nodes */
extern  int ni;             /* number of inputs */
extern  int no;             /* number of outputs */
extern  int nt;             /* nn + ni + 1 */
extern  int np;             /* ni + 1 */
extern  int backprop;       /* flag for back propagation */
extern  int localist;       /* flag for localist input */
extern  int teacher;        /* flag for feeding back target */

extern  CFStruct **cinfo;   /* (nn x nt) connection info */
extern  NFStruct *ninfo;    /* (nn) node info */



int act_nds(float *aold, float *amem,
            float *anew, float **awt,
            int   *local, float *atarget)
{
    register    int     i;
    register    int     j;
    register    int     *l;
    register    int     tcnt;

    register    float   *w;
    register    float   *zo;
    register    float   *zn;
    register    float   *zp;

    /* ************************* */
    register    float   *rzo;
    /* ************************* */

    register    float   **wp;
    register    float   *t;

    register    CFStruct **cp;
    register    NFStruct *n;
    register    NFStruct *on;
    register    CFStruct *ci;



    /* for each of nn nodes: update activations */

    /* remember current aold in amem */
    zo = aold + np;
    zn = amem + np;
    for (i = 0; i < nn; i++, zo++, zn++){
        *zn = *zo;
    }

    /* for each of nn nodes: update net inputs */
    n = ninfo;
    cp = cinfo;
    wp = awt;
    zn = anew + np;
    zp = aold + np;
    t = atarget;
    tcnt = 0;
    for (i = 0; i < nn; i++, zn++, n++, cp++, wp++, zp++){
        if (localist){
            rzo = amem;
            ci = *cp + np;
            w = *wp + np;
            zo = aold + np;
            *zn = **wp;
            l = local;
            while (*l != 0){
                *zn += *(*wp + *l++);
            }
            if (teacher){
                on = ninfo;
                for (j = 0; j < nn; rzo++, j++, w++,
                     zo++, ci++, on++){
                    if (ci->con){
                        if (on->targ){
                            if (*t == -9999.) { /* don't care */
                                *zn += *w * *zo;
                            } else {
                                *zn += *w * *t++;
                            }
                            if (++tcnt > no){
                                report_condition("Teacher forcing requires each output feeding exactly one node.",2);
                                return 1;
                            }
                        }
                        else
                            /* REC STUFF */
                            if(ci->rec==1) { 
                                    *zn += *w * *rzo;
                            } else {
                                    *zn += *w * *zo;
                            }
                    }
                }
            }
            else {
                for (j = 0; j < nn; j++, w++, zo++, ci++, rzo++){
                    if (ci->con) {
                        if(ci->rec==1) {
                                *zn += *w * *rzo;
                        } else { 
                                *zn += *w * *zo;
                        }
                    }
                }
            }
            /* apply activation function        */
            /*  0 = default             */
            /*  1 = bipolar             */
            /*  2 = linear              */
            /*  3 = binary              */
            if (n->func != 2){
                if (*zn > 10.)
                    *zn = 10.;
                else if (*zn < -10.)
                    *zn = -10.;
            }
            if (n->func < 2)
                *zn = 1.0 / (1.0 + EXP(0.0 - *zn));
            if (n->func == 1)
                *zn = 2. * *zn - 1.;
            else if (n->func == 3){
                if (*zn > 0.)
                    *zn = 1.;
                else
                    *zn = -1.;
            }
            /* if no-delay, then update immediately */
            if (n->dela == 0)
                *zp = *zn;
        }
        else {
            rzo = amem;
            ci = *cp;
            w = *wp;
            zo = aold;
            *zn = 0.;
            /* collect excitation */
            if (teacher){
                for (j = 0; j <= ni; j++, w++, rzo++, zo++, ci++){
                    if (ci->con)
                        *zn += *w * *zo;
                }
                on = ninfo;
                for (j = 0; j < nn; j++, w++, rzo++, zo++, ci++, on++){
                    if (ci->con){
                        if (on->targ){
                            if (*t == -9999.) {
                                if(ci->rec==1) *zn += *w * *rzo;
                                else *zn += *w * *zo;
                            }
                            else
                                *zn += *w * *t++;
                            if (++tcnt > no){
                                report_condition("Teacher forcing requires each output feeding exactly one node.",2);
                                return 1;
                            }
                        }
                        else {
                            if(ci->rec==1) *zn += *w * *rzo;
                            else *zn += *w * *zo;
                        }
                    }
                }
            }
            else {
                for (j = 0; j < nt; j++, w++, rzo++, zo++, ci++){
                    if (ci->con) {
                        if(ci->rec==1) *zn += *w * *rzo;
                        else *zn += *w * *zo;
                    }
                }
            }
            /* apply activation function        */
            /*  0 = default             */
            /*  1 = bipolar             */
            /*  2 = linear              */
            /*  3 = binary              */
            if (n->func != 2){
                if (*zn > 10.)
                    *zn = 10.;
                else if (*zn < -10.)
                    *zn = -10.;
            }
            if (n->func < 2)
                *zn = 1.0 / (1.0 + EXP(0.0 - *zn));
            if (n->func == 1)
                *zn = 2. * *zn - 1.;
            else if (n->func == 3){
                if (*zn > 0.)
                    *zn = 1.;
                else
                    *zn = -1.;
            }
            /* if no-delay, then update immediately */
            if (n->dela == 0)
                *zp = *zn;
        }
    }

    return 0;
}


