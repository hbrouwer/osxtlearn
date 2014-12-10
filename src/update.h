#ifndef UPDATE_H
#define UPDATE_H


int init_inputs(long *maxtime);
int update_inputs(float *aold, int tick, int flag, long *maxtime, int **local);
int init_targets(void);
int update_targets(float *atarget, long time, int tick, int flag, long *maxtime);
int init_reset(void);
int update_reset(long time, int tick, int flag, long *maxtime, int *now);
int update_weights(float **awt, float **adwt, float **awinc);


#endif
