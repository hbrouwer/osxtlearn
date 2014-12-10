/*
        xtlearnArchGlue.c
        All X specific functions for drawing are in this file.
*/

#include <math.h>
#include "arch.h"
#include "general.h"
#include "error.h"
#include "general_X.h"
#include "xtlearnArchGlue.h"

extern Display *appDisplay;
extern GC appGC, revGC;
extern Pixmap actPixmap, archPixmap, wtsPixmap;
extern Node *network_nodes;

extern int
        input_node_count, 
        show_node_numbers, 
        square_size_x, 
        square_size_y;


static char label[20];


        /* Draws a rectangle. */
void draw_architecture_rectangle(x, y, w, h, node_count, node_type, start_node)
     int x, y, w, h, node_count, node_type, start_node;
{
    x += DISPLAY_MARGIN;
    y += DISPLAY_MARGIN;

    XDrawRectangle(appDisplay, archPixmap, appGC, x, y, w, h);
    XFillRectangle(appDisplay, archPixmap, revGC, x+1, y+1, w-1, h-1);

    if (show_node_numbers) {
        switch(node_type) {
            case HIDDEN_NODE:
                if (node_count > 1)
                    sprintf(label, "%i-%i", start_node -input_node_count, 
                            start_node+node_count -1 -input_node_count);
                else sprintf(label, "%i", start_node -input_node_count);
                break;

            case INPUT_NODE:
                if (node_count > 1)
                    sprintf(label, "i%i-i%i", start_node, start_node+node_count -1);
                else sprintf(label, "i%i", start_node);
                break;

            case OUTPUT_NODE:
                if (node_count > 1)
                    sprintf(label, "o%i-o%i", start_node -input_node_count, 
                            start_node -input_node_count+node_count -1);
                else sprintf(label, "o%i", start_node -input_node_count);
                break;

            case BIAS_NODE:
                sprintf(label, "b");
                break;
        }

        XDrawImageString(appDisplay, archPixmap, appGC, 
                         x+5, y+12, label, strlen(label));
    }
}


        /* Draws a line on the arch display. */
void architecture_draw_line(int from_x, int from_y, int to_x, int to_y)
{
    XDrawLine(appDisplay, archPixmap, appGC, from_x, from_y, to_x, to_y);
}


        /* Sets certain line drawing characteristics. */
void architecture_set_line_attributes(int thickness, int line_style)
{
    if (line_style == DASHED_LINE)
        XSetLineAttributes(appDisplay, appGC, thickness, LineOnOffDash, 0, 0);
    else if (line_style == SOLID_LINE)
        XSetLineAttributes(appDisplay, appGC, thickness, LineSolid, 0, 0);
}


        /* Draws an arc. "end_angle" is relative to "start_angle". */
void architecture_draw_arc(int x, int y, int width, int height, int start_angle, int end_angle)
{
    XDrawArc(appDisplay, archPixmap, appGC, x, y, 
             width, height, start_angle, end_angle);
}



        /* Draws the given node. */
void draw_node(Node *node, int disp, float size, float max_size)
{
    float size_x;
    int x, y;

    x= node->x -(NODE_SIZE/2) +DISPLAY_MARGIN;
    y= node->y -(NODE_SIZE/2) +DISPLAY_MARGIN;

    if (disp == SPECIFICATION) { /* Draw architecture specification: */
        if (node->kind == BIAS_NODE) /* Draw bias node bigger than others. */
            XFillRectangle(appDisplay, archPixmap, appGC, 
                           x-2, y-2, NODE_SIZE+4, NODE_SIZE+4);

        else if (node->kind == INPUT_NODE) 
            XFillRectangle(appDisplay, archPixmap, appGC, 
                           x, y, NODE_SIZE, NODE_SIZE);

        else 
            XFillArc(appDisplay, archPixmap, appGC, 
                           x, y, NODE_SIZE, NODE_SIZE, 0, 360*64);

        if (show_node_numbers) {
            if (node->kind == HIDDEN_NODE) 
                sprintf(label, "%i", node->id - input_node_count);
            else if (node->kind == INPUT_NODE) 
                sprintf(label, "i%i", node->id);
            else if (node->kind == OUTPUT_NODE) 
                sprintf(label, "o%i", node->id - input_node_count);
            else if (node->kind == BIAS_NODE) 
                strcpy(label, "b");
            XDrawImageString(appDisplay, archPixmap, appGC, 
                             node->x, node->y, label, strlen(label));
        }
    }

     else if (disp == ACTIVATIONS) { /* Draw Hinton-like activations diagram: */
         XSetFillStyle(appDisplay, appGC, FillOpaqueStippled);
         XFillRectangle(appDisplay, actPixmap, appGC, x, y, NODE_SIZE, NODE_SIZE);
         XSetFillStyle(appDisplay, appGC, FillSolid);

         size_x= NODE_SIZE * (size/max_size) *1.05; /* round up slightly */
                        
         if (size_x < 0.0) size_x= -size_x; /* use absolute value */
                        
         x= node->x -(int)(size_x/2.0) +DISPLAY_MARGIN;
         y= node->y -(int)(size_x/2.0) +DISPLAY_MARGIN;
            
         if (size < 0.0) 
             XFillRectangle(appDisplay, actPixmap, appGC, x, y, (int)size_x, (int)size_x);
         else {
             XFillRectangle(appDisplay, actPixmap, revGC, x, y, (int)size_x, (int)size_x);
             XDrawRectangle(appDisplay, actPixmap, appGC, x, y, (int)size_x, (int)size_x);
         }
     }
}



void draw_weight_box(float size, float max_size, int x, int y) 
{
    float size_x, size_y;

    XSetFillStyle(appDisplay, appGC, FillOpaqueStippled);
    XFillRectangle(appDisplay, wtsPixmap, appGC, x, y, square_size_x, square_size_y);
    XSetFillStyle(appDisplay, appGC, FillSolid);

    size_x= square_size_x * (size/max_size);
    size_y= square_size_y * (size/max_size);

    if (size < 0.0) size_x= -size_x, size_y= -size_y;

    x += (square_size_x -(int)size_x)/2;
    y += (square_size_y -(int)size_y)/2;

    if (size < 0.0)             /* Negative activation... */
        XFillRectangle(appDisplay, wtsPixmap, appGC, x, y, (int)size_x, (int)size_y);
    else {                      /* positive activation... */
        XFillRectangle(appDisplay, wtsPixmap, revGC, x, y, (int)size_x, (int)size_y);
        XDrawRectangle(appDisplay, wtsPixmap, appGC, x, y, (int)size_x, (int)size_y);
    }
    
}


void weights_disp_label(int no, int x, int y, int input)
{
    if (!input) sprintf(label, "%d", no);
    else if (no > 0) sprintf(label, "i%d", no);
    else strcpy(label, "b");

    XDrawString(appDisplay, wtsPixmap, appGC, x, y, label, strlen(label));
}


void draw_line_arrowhead(short from_x, short from_y, short to_x, short to_y, short drawLine) 
{
    long delta_y, delta_x;
    float distance;
    float line_sin, line_cos;
    float butt_x, butt_y;
    XPoint points[3];

    if (from_x == to_x && from_y == to_y) return;

    delta_x= to_x- from_x;
    delta_y= to_y- from_y;

    distance= (float)sqrt(delta_x *delta_x +delta_y *delta_y);

    line_sin= (float)delta_y /distance;
    line_cos= (float)delta_x /distance;

    butt_x= (float)to_x -(line_cos *ARROW_LENGTH);
    butt_y= (float)to_y -(line_sin *ARROW_LENGTH);

    if (drawLine) 
        XDrawLine(appDisplay, archPixmap, appGC, 
                  from_x, from_y, (int)butt_x, (int)butt_y);

    points[0].x= to_x;
    points[0].y= to_y;
    points[1].x= butt_x -(line_sin *ARROW_WIDTH);
    points[1].y= butt_y +(line_cos *ARROW_WIDTH);
    points[2].x= butt_x +(line_sin *ARROW_WIDTH);
    points[2].y= butt_y -(line_cos *ARROW_WIDTH);

    XFillPolygon(appDisplay, archPixmap, appGC, 
                 points, 3, Convex, CoordModeOrigin);
}




