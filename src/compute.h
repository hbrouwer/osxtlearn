#ifndef COMPUTE_H
#define COMPUTE_H


int comp_errors(float *aold, float *atarget, float *aerror, float *e, float *ce_e);
int comp_backprop(float **awt, float **adwt, 
		  float *aold, float *amem, 
		  float *atarget, float *aerror, 
		  int *local);
int init_compute(void);


#endif
