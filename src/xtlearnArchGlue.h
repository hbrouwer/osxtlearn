#ifndef GLUE_H
#define GLUE_H

/* xtlearnArchGlue.h */

#include "arch.h"  /* Needed for Node typedef */

void draw_architecture_rectangle(int x, int y, int w, int h, int node_count, int node_type, int start_node);
void architecture_draw_line(int from_x, int from_y, int to_x, int to_y);
void architecture_draw_shape(int x, int y, int number_of_vertices);
void architecture_set_line_attributes(int thickness, int line_style);
void architecture_draw_arc(int x, int y, int width, int height, int start_angle, int end_angle);
void draw_node(Node *node, int which_display, float size, float max_size);
void draw_line_arrowhead(short from_x, short from_y, short to_x, short to_y, short drawLine);
void draw_weight_box(float size, float max_size, int x, int y);
void weights_disp_label(int x, int y, int no, int input);

#endif
