/*
	architecture.h
*/


#ifndef ARCH_H
#define ARCH_H




/* ----------------- */


typedef struct {
	int	kind;				/* input/hidden/output */
	int	id;				/* numeric tlearn id. */
	int	x, y;				/* screen coords. */
	int	layer;				/* What layer are we in? */
	int	slab;				/* What slab are we in? */
} Node;







	/* Node kinds: */
#define BIAS_NODE 0
#define INPUT_NODE 1
#define HIDDEN_NODE 2
#define OUTPUT_NODE 3



	/* Architecture orientations: */
#define ARCHITECTURE_VERTICAL 1
#define ARCHITECTURE_HORIZONTAL 0
#define DEFAULT_ARCHITECTURE_ORIENTATION 1

	/* displays: */

#ifndef SPECIFICATION
#define SPECIFICATION 0
#define ACTIVATIONS 1
#define WEIGHTS 2
#endif


	/* Connection types: */
#define NORMAL_CONNECTION 0
#define LOW_TO_HIGH_CONNECTION 1
#define HIGH_TO_LOW_CONNECTION 2
#define SELF_CONNECTION 3

#define RESOLVED_CONNECTION 4


	/* General error information: */
#define FATAL_ERROR 3
#define NO_LINE_NUMBER 0



	/* Various constants which affect graphical output;
	   these are set for X Windows - others may need to
	   change them. */
#define	ARC_ANGLE_RESOLUTION 64	/* Angle minute resolution for arc drawing - may be 1 on other systems. */
#define	DASHED_LINE	0	
#define	SOLID_LINE	1
#define MAX_ARROW_HEAD_VERTICES 5
#define	ARROW_LENGTH	14.0	/* Length of arrowhead on connections. */
#define	ARROW_WIDTH	4.0	/* Width of base of arrowhead on connections. */
#define	NODE_SIZE	15	/* Default node size */

#define	THICK_LINE	3

#ifdef __QUICKDRAW__
#define	THIN_LINE	1	
#else  /* XTLEARN */
#define	THIN_LINE	0	/* May be 1 on other systems. */
#endif

#define DISPLAY_MARGIN 5	/* Border margin around display. */
#define CURVE_SCALAR 4		/* How curved are curved connections?
				   Bigger numbers = thinner curves */


int make_new_layer(int *node_array);
int make_new_slab(int *node_array);
int prepare_architecture_memory(int node_count);
int display_architecture(int which_display);

#endif
