#include "error.h"
#include "general_X.h"
#include "arch.h"
#include "xtlearn.h"
#include "xtlearnActDisp.h"


extern GC appGC, revGC;
extern Display *appDisplay;
extern int appDepth, gRunning;
extern Cursor waitCursor;
extern learning, verify, probe, randomly;
extern long tmax;

Pixmap actPixmap; 
Widget actViewport;

static void CloseActDisplay(Widget w, XtPointer client_data, XtPointer call_data);
static void NextStepActDisplay(Widget w, XtPointer client_data, XtPointer call_data);
static void aPaintActDisplay(Widget w, XEvent *event, String *params, Cardinal *count);

static Window actWindow;

static Widget
    actShell,
    actForm,
    actClose,
    actFirstButton,
    actNextButton;

static int patternNo;
static Dimension width, height;
static char *apologetic_message= "Sorry, the Activations Display cannot be used during training, verifying or probing.";
static char buf[20];

static XtActionsRec actions[]= {
    {"aPaintActDisplay", aPaintActDisplay },
    {"aUpdateActDisplay", aUpdateActDisplay }
};


void InitActDisplay(XtAppContext appContext, Widget appWidget)
{
    XtAppAddActions(appContext, actions, XtNumber(actions));

    actShell= XtCreatePopupShell("Node Activations",
        topLevelShellWidgetClass, appWidget, NULL, 0);

    actForm= XtVaCreateManagedWidget("actForm", 
        formWidgetClass, actShell,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        XtNdefaultDistance, 0,
        NULL);

    actClose= XtVaCreateManagedWidget("actClose", 
        commandWidgetClass, actForm, LOCK_GEOMETRY,
        XtNhorizDistance, 4,
        XtNvertDistance, 2,
        XtNlabel, " Close ",
        NULL);

    actFirstButton= XtVaCreateManagedWidget("actFirstButton", 
        commandWidgetClass, actForm, LOCK_GEOMETRY,
        XtNfromHoriz, actClose,
        XtNhorizDistance, 8,
        XtNvertDistance, 2,
        XtNlabel, " First Data Pattern ",
        NULL);

    actNextButton= XtVaCreateManagedWidget("actNextButton", 
        commandWidgetClass, actForm, LOCK_GEOMETRY,
        XtNfromHoriz, actFirstButton,
        XtNhorizDistance, 8,
        XtNvertDistance, 2,
        XtNlabel, " Next Data Pattern ",
        NULL);

    actViewport= XtVaCreateManagedWidget("actViewport",
        coreWidgetClass, actForm,
        XtNtranslations, XtParseTranslationTable(
            "<Configure>: aUpdateActDisplay() \n\
             <Expose>:    aPaintActDisplay()"),
        XtNtop, XtChainTop, 
        XtNleft, XtChainLeft,
        XtNright, XtChainRight,
        XtNvertDistance, 2,
        XtNborderWidth, 0,
        XtNfromVert, actClose,
        XtNwidth, DISPLAY_DEFAULT_WIDTH,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        NULL);

    XtAddCallback(actClose, XtNcallback, CloseActDisplay, NULL);
    XtAddCallback(actFirstButton, XtNcallback, ResetActDisplay, NULL);
    XtAddCallback(actNextButton, XtNcallback, NextStepActDisplay, NULL);
}


void PopupActDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (init_config()) return;
    XtPopup(actShell, XtGrabNone);
    actWindow= XtWindow(actViewport);
    XtSetSensitive(actFirstButton, 1);
    XtSetSensitive(actNextButton, 0);
    patternNo= 0;
    aUpdateActDisplay(0, 0, 0, 0);
}


void aUpdateActDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (!XtIsRealized(actViewport)) return;

    XtVaGetValues(actViewport, XtNwidth,  &width,  XtNheight, &height, NULL);

    if (actPixmap) XFreePixmap(appDisplay, actPixmap);
    actPixmap= 0;
    actPixmap= XCreatePixmap(appDisplay, actWindow, width, height, appDepth);

    if (actPixmap == 0) {
        CloseActDisplay(0, 0, 0);
        memerr("Offscreen activations");
        return;
    }

    XDefineCursor(appDisplay, actWindow, waitCursor);
    XFillRectangle(appDisplay, actPixmap, revGC, 0, 0, width, height);
    XDrawLine(appDisplay, actPixmap, appGC, 0, 0, width +10, 0);
    if (display_architecture(ACTIVATIONS)) CloseActDisplay(0,0,0);
    if (patternNo) {
        sprintf(buf, "pattern: %d", patternNo);
        XDrawString(appDisplay, actPixmap, appGC, 
                    DISPLAY_MARGIN, height -DISPLAY_MARGIN, buf, strlen(buf));
    }
    aPaintActDisplay(0, 0, 0, 0);
    XUndefineCursor(appDisplay, actWindow);
}


static void aPaintActDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    XCopyArea(appDisplay, actPixmap, actWindow, appGC, 0, 0, 
              width, height, 0, 0);
}


static void CloseActDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (actPixmap) XFreePixmap(appDisplay, actPixmap);
    actPixmap= 0;
    XtPopdown(actShell);
}


static void NextStepActDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (gRunning) {
        report_condition(apologetic_message, 1);
        return;
    }

    verify = 0;
    learning= 0;
    randomly= 0;
/*  probe= 1;   /* Displays selected nodes if output window is open */

    patternNo++;
    if (patternNo > tmax) patternNo= 1;

    XTlearnLoop();
/*  probe= 0; */
    XtSetSensitive(actFirstButton, 1);
    aUpdateActDisplay(0, 0, 0, 0);
}


void ResetActDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (!actPixmap) return;
    if (gRunning) {
        report_condition(apologetic_message, 1);
        return;
    }

    probe= 1;    /* Not really probing, this just sets up the wts/data files */
    verify= 0;
    learning= 0;
    randomly= 0;

    patternNo= 1;

    if (set_up_simulation()) return;
    probe= 0;
    XTlearnLoop();
    XtSetSensitive(actFirstButton, 0);
    XtSetSensitive(actNextButton, 1);
    aUpdateActDisplay(0, 0, 0, 0);
}


int ActDispWidth(void)
{
    return (int)width;
}


int ActDispHeight(void)
{
    return (int)height;
}
