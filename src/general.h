#ifndef GENERAL_H
#define GENERAL_H

typedef struct {
	int con;	/* connection flag */
	int fix;	/* fixed-weight flag */
	int num;	/* group number */
	int lim;	/* weight-limits flag */
	int rec;	/* bptt forced recurrence flag */
	float min;	/* weight minimum */
	float max;	/* weight maximum */
	} CFStruct;

typedef struct {
	int func;	/* activation function type */
	int dela;	/* delay flag */
	int targ;	/* target flag */
	} NFStruct;

#endif
