#ifndef SUBS_H
#define SUBS_H

int exp_init(void);
int print_nodes(float *aold);
int print_output(float *aold);
int print_error(float *e, int sweep);
int reset_bp_net(float *aold, float *anew, float *amem);
float rans(double limit);

#endif
