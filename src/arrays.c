
/* make_arrays() - calloc space for arrays */

#include <stdlib.h>
#include "general.h"
#include "error.h"
#include "arrays.h"


#ifdef __QUICKDRAW__
/* MACINTOSH VERSION */
/* IMPORTANT:  For big multidimensional arrays */
/* we can't trust the calloc() C lib function. */
/* Mac_calloc() allocates with the mac toolbox */
/* function NewPtr(), so we use DisposPtr() in */
/* place of free(). */

#define calloc Mac_calloc       
#define free(x) DisposPtr((Ptr)(x)) 
void *Mac_calloc(size_t count, size_t size);

#endif


extern  int     nn;             /* number of nodes */
extern  int     ni;             /* number of inputs */
extern  int     no;             /* number of outputs */
extern  int     nt;             /* nn + ni + 1 */

extern  CFStruct        **cinfo;        /* (nn x nt) connection info */
extern  NFStruct        *ninfo;         /* (nn) node activation function info */

extern  int     *outputs;       /* (no) indices of output nodes */
extern  int     *selects;       /* (nn+1) nodes selected for probe printout */
extern  int     *linput;        /* (ni) localist input array */

extern  float   *znew;          /* (nt) inputs and activations at time t+1 */
extern  float   *zold;          /* (nt) inputs and activations at time t */
extern  float   *zmem;          /* (nt) inputs and activations at time t */
extern  float   **wt;           /* (nn x nt) weight TO node i FROM node j*/ 
extern  float   **dwt;          /* (nn x nt) delta weight at time t */
extern  float   **winc;         /* (nn x nt) accumulated weight increment*/
extern  float   *target;        /* (no) output target values */
extern  float   *error;         /* (nn) error= (output - target) values */

static int nn_save;   /* Needed for freeing the multidimensional arrays */

static void free_arrays(void);

int make_arrays(void)
{
    int i;
    int j;

    CFStruct    *ci;
    NFStruct    *n;

    free_arrays();

    nn_save= nn;

    zold= (float *) calloc(nt, sizeof(float));
    if (zold == NULL) return memerr("zold");

    zmem= (float *) calloc(nt, sizeof(float));
    if (zmem == NULL) return memerr("zmem");

    znew= (float *) calloc(nt, sizeof(float));
    if (znew == NULL) return memerr("znew");

    target= (float *) calloc(no, sizeof(float));
    if (target == NULL) return memerr("target");

    error= (float *) calloc(nn, sizeof(float));
    if (error == NULL) return memerr("error");

    selects= (int *) calloc(nt, sizeof(int));
    if (selects == NULL) return memerr("selects");

    outputs= (int *) calloc(no, sizeof(int));
    if (outputs == NULL) return memerr("outputs");

    linput= (int *) calloc(ni, sizeof(int));
    if (linput == NULL) return memerr("linput");

    wt= (float **) calloc(nn, sizeof(float *));
    if (wt == NULL) return memerr("wt");

    for (i= 0; i < nn; i++){
        *(wt + i)= (float *) calloc(nt, sizeof(float));
        if (*(wt + i) == NULL) return memerr("wt");
    }

    dwt= (float **) calloc(nn, sizeof(float *));
    if (dwt == NULL) return memerr("dwt");

    for (i= 0; i < nn; i++){
        *(dwt + i)= (float *) calloc(nt, sizeof(float));
        if (*(dwt + i) == NULL) return memerr("dwt");
    }

    winc= (float **) calloc(nn, sizeof(float *));
    if (winc == NULL) return memerr("winc");

    for (i= 0; i < nn; i++){
        *(winc + i)= (float *) calloc(nt, sizeof(float));
        if (*(winc + i) == NULL) return memerr("winc");
    }

    cinfo= (CFStruct **) calloc(nn, sizeof(CFStruct *));
    if (cinfo == NULL) return memerr("cinfo");

    for (i= 0; i < nn; i++){
        *(cinfo + i)= (CFStruct *) calloc(nt, sizeof(CFStruct));
        if (*(cinfo + i) == NULL) return memerr("cinfo");
    }

    ninfo= (NFStruct *) calloc(nn, sizeof(NFStruct));
    if (ninfo == NULL) return memerr("ninfo");

    n= ninfo;
    for (i= 0; i < nn; i++, n++){
        n->func= 0;
        n->dela= 0;
        n->targ= 0;
        ci= *(cinfo + i);
        for (j= 0; j < nt; j++, ci++){
            ci->con= 0;
            ci->fix= 0;
            ci->num= 0;
            ci->lim= 0;
            ci->min= 0.;
            ci->max= 0.;
        }
    }

    return 0;
}



void free_arrays(void)
{
    int i;

    if (ninfo) free(ninfo);
    ninfo= NULL;

    if (cinfo) {
        for (i= nn_save -1; i >= 0; i--) {
            if (*(cinfo + i)) free(*(cinfo + i));
            *(cinfo + i)= NULL;
        }
        free(cinfo);
    }
    cinfo= NULL;

    if (winc) {
        for (i= nn_save -1; i >= 0; i--) {
            if (*(winc + i)) free(*(winc + i));
            *(winc + i)= NULL;
        }
        free(winc);
    }
    winc= NULL;

    if (dwt) {
        for (i= nn_save -1; i >= 0; i--) {
            if (*(dwt + i)) free(*(dwt + i));
            *(dwt + i)= NULL;
        }

        free(dwt);
    }
    dwt= NULL;

    if (wt) {
        for (i= nn_save -1; i >= 0; i--) {
            if (*(wt + i)) free(*(wt + i));
            *(wt + i)= NULL;
        }
        free(wt);
    }
    wt= NULL;


    if (linput) free(linput);
    if (outputs) free(outputs);
    if (selects) free(selects);
    if (error) free(error);
    if (target) free(target);
    if (znew) free(znew);
    if (zmem) free(zmem);
    if (zold) free(zold);

    linput= NULL;
    outputs= NULL;
    selects= NULL;
    error= NULL;
    target= NULL;
    znew= NULL;
    zmem= NULL;
    zold= NULL;
}
