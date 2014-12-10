#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "subs.h"
#include "weights.h"

#ifdef  EXP_TABLE
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

float   *exp_array;     /* table look-up for exp function */
float   exp_mult;
long    exp_add;
#endif  /* EXP_TABLE */


extern  int     nn;             /* number of nodes */ 
extern  int     ni;             /* number of inputs */
extern  int     no;             /* number of outputs */
extern  int     nt;             /* nn + ni + 1 */

extern  int     ce;             /* cross-entropy flag */

extern  int     *outputs;       /* (no) indices of output nodes */
extern  int     *selects;       /* (nn+1) nodes selected for probe printout */

extern  char    root[128];      /* root filename for .cf, .data, .teach, etc.*/

extern  long    sweep;          /* current sweep */
extern  long    rms_report;     /* report error every report sweeps */

extern  float   criterion;      /* exit program when rms error < criterion */


/* Since some machines don't define a RAND_MAX and some rand() functions */
/* return a full 4 byte int max value, we'll strip off the 2 high bytes  */
/* and force the max to be 32767.  This should make rans() more portable. */

float rans(double limit)
{
    return 2* limit* (rand() & 32767)/ 32767 -limit;
}


int exp_init(void)
{
#ifdef  EXP_TABLE
    struct  stat statb;
    int     fd;

    fd = open(EXP_TABLE, O_RDONLY, 0);
    if (fd < 0)
        return report_condition("exp_table "EXP_TABLE" not found.", 3);

    fstat(fd, &statb);
    exp_add = (statb.st_size / sizeof(float)) / 2;
    exp_mult = (float) (exp_add / 16);
    exp_array = (float *) calloc(1,statb.st_size);
    if (read(fd, exp_array, statb.st_size) != statb.st_size)
        return report_condition("can't read exp array", 3);

#endif /* EXP_TABLE */
    return 0;
}


int print_nodes(float *aold) 
{
    int i;

    for (i = 1; i <= nn; i++){
        if (selects[i])
            fprintf(stdout,"%7.3f\t",aold[ni+i]);
    }
    fprintf(stdout,"\n");

    return 0;
}


int print_output(float *aold)
{
    int     i;

    for (i = 0; i < no; i++){
        fprintf(stdout,"%7.3f\t",aold[ni+outputs[i]]);
    }
    fprintf(stdout,"\n");
    return 0;
}


int print_error(float *e, int sweep)
{
    static      int     start = 1;
    static      FILE    *fp;
    char        file[128];

    if (start){
        start = 0;
        sprintf(file, "%s.err", root);
        fp = fopen(file, "w");
        if (fp == NULL) {
            report_condition("Can't open error file.",2);
            return 1;
        }
    }

    if (ce != 2) {
        /* report rms error */
        *e = sqrt(*e / rms_report);
    } else if (ce == 2) {
        /* report cross-entropy */
        *e = *e / rms_report;
    }
    fprintf(fp,"%g\n",*e);
    fflush(fp);
    if (ce == 0) {
        if (*e < criterion){
            sweep += 1;
            save_wts();
            exit(0);
        }
    }
    *e = 0.;
    return 0;
}


#ifdef NOT_USED
int reset_network(aold,anew,apold,apnew)
        float   *aold;
        float   *anew;
        float   ***apold;
        float   ***apnew;
{
    register        int     i, j, k;

    register        float   *pn;
    register        float   *po;
    register        float   **pnp;
    register        float   **pop;
    register        float   ***pnpp;
    register        float   ***popp;

    register        float   *zn;
    register        float   *zo;

    zn = anew + 1;
    zo = aold + 1;
    for (i = 1; i < nt; i++, zn++, zo++)
        *zn = *zo = 0.;

    popp = apold;
    pnpp = apnew;
    for (i = 0; i < nn; i++, popp++, pnpp++){
        pop = *popp;
        pnp = *pnpp;
        for (j = 0; j < nt; j++, pop++, pnp++){
            po = *pop;
            pn = *pnp;
            for (k = 0; k < nn; k++, po++, pn++){
                *po = 0.;
                *pn = 0.;
            }
        }
    }

    return 0;
}
#endif


int reset_bp_net(float *aold, float *anew, float *amem)
{
    register    int     i;
    register    float   *zn;
    register    float   *zo;
    register    float   *zm;

    zn = anew + 1;
    zo = aold + 1;
    zm = amem + 1;
    for (i = 1; i < nt; i++, zn++, zo++)
        *zn = *zo = *zm = 0.;

    return 0;
}

