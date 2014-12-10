#include <math.h>
#include <stdio.h> 
#include <stdlib.h>
#include "error.h"
#include "general_X.h"
#include "xtlearnIface.h"
#include "xtlearnErrDisp.h"

#define ERROR_WIDTH_MARGIN 5
#define ERROR_HEIGHT_MARGIN 15

#define CURVE_OVERFLOW_BUFFER 1.3
#define CURVE_UNDERFLOW_BUFFER 1.3
#define DEFAULT_ERROR_POINTS 100

#define DEFAULT_INTERVAL 100

extern long nsweeps, tsweeps;
extern int ce, gAbort, learning;
extern char root[];
extern float criterion;

extern Pixmap
    checkedRB,
    uncheckedRB;

extern Display *appDisplay;
extern GC appGC, revGC;
extern int appDepth;
extern Cursor waitCursor;
extern TestingOptions TestOpts;
extern TrainingOptions TrainOpts;

int errInterval;

Pixmap errPixmap; 
Widget errViewport;


static void aErrDispToggles(Widget w, XEvent *event, String *params, Cardinal *count);
static void aPaintErrDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
static void CloseErrDisplay(Widget w, XtPointer client_data, XtPointer call_data);
static int draw_error_display(void);
static int plot_error(int index, int mirror);

static char buf[40];
static FILE *fp;
static double xMultiplier, yMultiplier;  /* for value to pixel conversions */
static float errorMax;
static float *errValues= NULL;
static int
        errValueCount,
        maxErrorValues= 0,
        xMax,
        yMax,
        error_points_are_connected;

static Dimension width, height;

static Window errWindow;

static Widget
    errShell,
    errForm,
    errClose,
    errLineRB,
    errLineRBLabel,
    errPointsRB,
    errPointsRBLabel;

static XtActionsRec actions[]= {
    {"aErrDispToggles", aErrDispToggles },
    {"aPaintErrDisplay", aPaintErrDisplay },
    {"aUpdateErrDisplay", aUpdateErrDisplay }
};


void InitErrDisplay(XtAppContext appContext, Widget appWidget)
{
    XtTranslations toggleTrans;

    XtAppAddActions(appContext, actions, XtNumber(actions));

    toggleTrans= XtParseTranslationTable(
        "<Leave>:            reset()    \n\
         <BtnDown>,<BtnUp>: aErrDispToggles()");

    errShell= XtCreatePopupShell("Error Display",
        topLevelShellWidgetClass, appWidget, NULL, 0);

    errForm= XtVaCreateManagedWidget("errForm", 
        formWidgetClass, errShell,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        XtNdefaultDistance, 0,
        NULL);

    errClose= XtVaCreateManagedWidget("errClose", 
        commandWidgetClass, errForm, LOCK_GEOMETRY,
        XtNhorizDistance, 4,
        XtNvertDistance, 2,
        XtNlabel, " Close ",
        NULL);

    errLineRB= XtVaCreateManagedWidget("errLineRB",
        toggleWidgetClass, errForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, errClose,
        XtNhorizDistance, 8,
        XtNvertDistance, 4,
        XtNbitmap, uncheckedRB,
        NULL);

    errLineRBLabel= XtVaCreateManagedWidget("errLineRBLabel",
        toggleWidgetClass, errForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, errLineRB,
        XtNhorizDistance, 0,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Line",
        NULL);

    errPointsRB= XtVaCreateManagedWidget("errPointsRB",
        toggleWidgetClass, errForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, errLineRBLabel,
        XtNhorizDistance, 8,
        XtNvertDistance, 4,
        XtNbitmap, checkedRB,
        NULL);

    errPointsRBLabel= XtVaCreateManagedWidget("errPointsRBLabel",
        toggleWidgetClass, errForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, errPointsRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Points",
        NULL);

    errViewport= XtVaCreateManagedWidget("errViewport",
        coreWidgetClass, errForm,
        XtNtranslations, XtParseTranslationTable(
            "<Configure>: aUpdateErrDisplay() \n\
             <Expose>:    aPaintErrDisplay()"),
        XtNtop, XtChainTop, 
        XtNleft, XtChainLeft,
        XtNright, XtChainRight,
        XtNvertDistance, 2,
        XtNborderWidth, 0,
        XtNfromVert, errClose,
        XtNwidth, 550,
        XtNheight, 275,
        NULL);

    XtAddCallback(errClose, XtNcallback, CloseErrDisplay, NULL);
}


static void aErrDispToggles(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (w == errLineRB || w == errLineRBLabel) {
        if (error_points_are_connected) return;
        SetRadioButtons(errPointsRB, errLineRB, NULL, error_points_are_connected= 1);
    }

    else if (w == errPointsRB || w == errPointsRBLabel) {
        if (!error_points_are_connected) return;
        SetRadioButtons(errPointsRB, errLineRB, NULL, error_points_are_connected= 0);
    }

    else return;

    aUpdateErrDisplay(0, 0, 0, 0);
}


void PopupErrDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtPopup(errShell, XtGrabNone);
    errWindow= XtWindow(errViewport);
    aUpdateErrDisplay(0, 0, 0, 0);
}


void aUpdateErrDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (!XtIsRealized(errViewport)) return;
    XtVaGetValues(errViewport, XtNwidth,  &width,  NULL);
    XtVaGetValues(errViewport, XtNheight, &height, NULL);

    if (errPixmap) XFreePixmap(appDisplay, errPixmap);
    errPixmap= 0;
    errPixmap= XCreatePixmap(appDisplay, errWindow, width, height, appDepth);

    if (errPixmap == 0) {
        CloseErrDisplay(0, 0, 0);
        memerr("error Pixmap");
        return;
    }
                
    XDefineCursor(appDisplay, errWindow, waitCursor);
    XFillRectangle(appDisplay, errPixmap, revGC, 0, 0, width, height);
    draw_error_display();
    aPaintErrDisplay(0, 0, 0, 0);
    XUndefineCursor(appDisplay, errWindow);
}
        

static void aPaintErrDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    XCopyArea(appDisplay, errPixmap, errWindow, appGC, 0, 0,
              width, height, 0, 0);
}


static void CloseErrDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (errPixmap) XFreePixmap(appDisplay, errPixmap);
    errPixmap= 0;
    XtPopdown(errShell);
}               


int AllocErrDisplay(void)
{
    errValueCount = 0;
    errorMax = 0;

    if (learning && TrainOpts.writeError) errInterval= TrainOpts.errorInterval;
    else if (!learning) errInterval= 1;
    else errInterval= DEFAULT_INTERVAL;
 
    if (errValues) {
        free(errValues);
        errValues= NULL;
    }

    maxErrorValues= 1+ (nsweeps -tsweeps)/errInterval;
    errValues= (float*)calloc(maxErrorValues, sizeof(float));
    if (errValues == NULL) 
        return report_condition("Not enough memory for error plot.  Reduce error logging frequency.", 2);

    if ((learning && TrainOpts.writeError) || (!learning && TestOpts.writeError)) {
        sprintf(buf, "%s.err", root);
        fp= fopen(buf, "a");
        if (fp == NULL) {
            if (learning) TrainOpts.writeError= 0;
            else TestOpts.writeError= 0;
            return report_condition("Can't open error file.", 2);
        }
    }
    return 0;
}




        /* This function is called by the simulation loop to
           report the next error value.   Error values are stored
           in the errValues array.  The errValueCount holds the 
           total number of points so far.  aUpdateErrDisp is called
           if new error value is out of range. */


void PlotErrPoint(float *err)
{
    float value;
        
    if (errValueCount +2 > maxErrorValues) return;
        
    if (ce != 2) value= sqrt(*err / errInterval);      /* report rms error */
    else value= -(*err / errInterval);            /* report cross-entropy */
                
    if (ce == 0 && value < criterion) gAbort= 1;

    if (errValues == NULL) return;

    errValues[errValueCount++]= value;

    if (value < errorMax) {
        if (errPixmap) plot_error(errValueCount -1, TRUE);
    }
    else {
        errorMax= value * 1.1;
        if (errPixmap) aUpdateErrDisplay(0, 0, 0, 0);
    }

    if ((learning && TrainOpts.writeError) || 
       (!learning && TestOpts.writeError)) {
        fprintf(fp, "%g\n", value);
        fflush(fp);
    }

    *err= 0.0;
}
    

#define ERROR_BOTTOM_MARGIN 20
#define ERROR_LEFT_MARGIN 50
#define ERROR_RIGHT_MARGIN 20
#define ERROR_TOP_MARGIN 6

        /* Code to completely re-paint the error display.  It is called
           when the error display is new, resized, or receives an expose
           event.  It is responsible for scaling, tick marks and labels. */

static int draw_error_display(void)
{
    int i, x, y, len, firsttime= 1;
    long sweepRange;                  /* total sweeps represented in plot */
    long tickXInterval;               /* sweeps between each X-axis tick mark */
    long tickXOffset;                 /* sweep value of first X-axis tick mark */
    double tickYInterval;             /* interval of Y-axis tick marks (no units) */

    yMax= height -ERROR_BOTTOM_MARGIN -ERROR_TOP_MARGIN;
    xMax= width -ERROR_LEFT_MARGIN -ERROR_RIGHT_MARGIN;

    XDrawRectangle(appDisplay, errPixmap, appGC, 
        ERROR_LEFT_MARGIN, ERROR_TOP_MARGIN, xMax, yMax);

    if (!learning && !TestOpts.calculateError) return 0;

    sweepRange= nsweeps -tsweeps;
    if (sweepRange < 1L) return 0;

    xMultiplier= (double)xMax/ (double)sweepRange;

    if (sweepRange > 3) {                   /* magic tick formula */
        tickXInterval= (float)pow(10., ceil(log10((double)sweepRange))) * 0.4;
        for (i= 0; i < 3; i++) if (sweepRange/(tickXInterval *= 0.5) > 3) break;
        if (i == 3) tickXInterval *= 0.4;
    }

    else tickXInterval= 1;

    tickXOffset= tickXInterval *(tsweeps/tickXInterval);

    for (i= 0; ; i++) {   /* draw the x-axis ticks */
        x= ((i *tickXInterval +tickXOffset) -tsweeps) * xMultiplier;
        if (x > xMax) break; 
        if (x < 0) continue;
        x += ERROR_LEFT_MARGIN;
        y= yMax +ERROR_TOP_MARGIN;
        XDrawLine(appDisplay, errPixmap, appGC, x, y, x, y -5);

        sprintf(buf, "%ld", i *tickXInterval +tickXOffset);
        len= strlen(buf);
        XDrawString(appDisplay, errPixmap, appGC, x -(len *3), y +12, buf, len);
        if (firsttime) {
            if (x > 40 + ERROR_LEFT_MARGIN) 
               XDrawString(appDisplay, errPixmap, appGC, ERROR_LEFT_MARGIN, y +12, "sweeps", 6);
            else XDrawString(appDisplay, errPixmap, appGC, x +(len *3) +6, y +12, "sweeps", 6);
            firsttime= 0;
        }
    }


    if (errorMax == 0.0) return 0;

    yMultiplier= (double)yMax/ (double)errorMax;

    tickYInterval= pow(10., ceil(log10((double)errorMax))) * 0.4; /* magic tick formula */
    for (i= 0; i < 3; i++) if (errorMax/(tickYInterval *= 0.5) >= 3.0) break;
    if (i == 3) tickYInterval *= 0.4;

                                                        /* draw the Y tics */
    for (i= 0; ; i++) {
        y= (yMultiplier *tickYInterval) *i;
        if (y > yMax) break;
        y= yMax +ERROR_TOP_MARGIN -y;
        XDrawLine(appDisplay, errPixmap, appGC, ERROR_LEFT_MARGIN, y, ERROR_LEFT_MARGIN +5, y);
        sprintf((char *) buf, "%4.4f", tickYInterval* (double)i);
        len= strlen(buf);
        XDrawString(appDisplay, errPixmap, appGC, 10, y+4, buf, len);
    }

    if (errValues == NULL) return 0;

    for (i= 0; i < errValueCount; i++)
        if (plot_error(i, FALSE)) return -1;

    return 0;
}


        /* Draws to errPixmap and errWindow if mirror is true. */

static int plot_error(int index, int mirror)
{
    int x1, y1, x2, y2;

    x1= (index +1) *errInterval *xMultiplier +ERROR_LEFT_MARGIN;
    y1= yMax +ERROR_TOP_MARGIN -errValues[index] *yMultiplier;

    if (error_points_are_connected && index > 0) {
        x2= index *errInterval *xMultiplier +ERROR_LEFT_MARGIN;
        y2= yMax +ERROR_TOP_MARGIN -errValues[index -1] *yMultiplier;
        XDrawLine(appDisplay, errPixmap, appGC, x2, y2, x1, y1);
        if (mirror) XDrawLine(appDisplay, errWindow, appGC, x2, y2, x1, y1);
    }
    else {
        XDrawRectangle(appDisplay, errPixmap, appGC, x1, y1, 2, 2);
        if (mirror) XDrawRectangle(appDisplay, errWindow, appGC, x1, y1, 2, 2);
    }
    return 0;
}

