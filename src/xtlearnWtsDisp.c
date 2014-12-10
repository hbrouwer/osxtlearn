#include "error.h"
#include "arch.h"
#include "weights.h"
#include "general_X.h"
#include "xtlearn.h"
#include "xtlearnIface.h"
#include "xtlearnWtsDisp.h"

extern Pixmap 
    checkedCB, uncheckedCB,
    checkedRB, uncheckedRB;

extern GC appGC, revGC;
extern Display *appDisplay;
extern Cursor waitCursor;
extern long sweep, tmax;
extern int appDepth, gRunning;

Pixmap wtsPixmap; 
Widget wtsViewport;

static void aWtsDispToggles(Widget w, XEvent *event, String *params, Cardinal *count);
static void aPaintWtsDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
static void RedrawWtsDisplay(Widget w, XtPointer client_data, XtPointer call_data);
static void CloseWtsDisplay(Widget w, XtPointer client_data, XtPointer call_data);
static void SetWtsDispUpdate(void);

static long interval, next_update;
static short epochs, animate;

static Window wtsWindow;

static Widget
    wtsShell,
    wtsForm,
    wtsClose,
    wtsRedraw,
    wtsUpdateCB,
    wtsUpdateCBLabel,
    wtsOneRB,
    wtsOneRBLabel,
    wtsFiveRB,
    wtsFiveRBLabel,
    wtsTenRB,
    wtsTenRBLabel,
    wtsSweepsRB,
    wtsSweepsRBLabel,
    wtsEpochsRB,
    wtsEpochsRBLabel;

static Dimension width, height;

static XtActionsRec actions[]= {
    {"aWtsDispToggles", aWtsDispToggles },
    {"aPaintWtsDisplay", aPaintWtsDisplay },
    {"aUpdateWtsDisplay", aUpdateWtsDisplay }
};


void InitWtsDisplay(XtAppContext appContext, Widget appWidget)
{
    XtTranslations toggleTrans;

    XtAppAddActions(appContext, actions, XtNumber(actions));

    toggleTrans= XtParseTranslationTable(
        "<Leave>:            reset()    \n\
         <BtnDown>,<BtnUp>: aWtsDispToggles()");

    wtsShell= XtCreatePopupShell("Connection Weights",
        topLevelShellWidgetClass, appWidget, NULL, 0);

    wtsForm= XtVaCreateManagedWidget("wtsForm", 
        formWidgetClass, wtsShell,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        XtNdefaultDistance, 0,
        NULL);

    wtsClose= XtVaCreateManagedWidget("wtsClose", 
        commandWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNhorizDistance, 4,
        XtNvertDistance, 2,
        XtNlabel, " Close ",
        NULL);

    wtsRedraw= XtVaCreateManagedWidget("wtsRedraw", 
        commandWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNfromHoriz, wtsClose,
        XtNhorizDistance, 8,
        XtNvertDistance, 2,
        XtNlabel, " Redraw ",
        NULL);


    wtsUpdateCB= XtVaCreateManagedWidget("wtsUpdateCB",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsRedraw,
        XtNhorizDistance, 20,
        XtNvertDistance, 4,
        XtNbitmap, checkedCB,
        NULL);

    wtsUpdateCBLabel= XtVaCreateManagedWidget("wtsUpdateCBLabel",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsUpdateCB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Update Every:",
        NULL);


    wtsOneRB= XtVaCreateManagedWidget("wtsOneRB",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsUpdateCBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, checkedRB,
        NULL);

    wtsOneRBLabel= XtVaCreateManagedWidget("wtsOneRBLabel",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsOneRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "1",
        NULL);


    wtsFiveRB= XtVaCreateManagedWidget("wtsFiveRB",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsOneRBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, uncheckedRB,
        NULL);

    wtsFiveRBLabel= XtVaCreateManagedWidget("wtsFiveRBLabel",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsFiveRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "5",
        NULL);


    wtsTenRB= XtVaCreateManagedWidget("wtsTenRB",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsFiveRBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, uncheckedRB,
        NULL);

    wtsTenRBLabel= XtVaCreateManagedWidget("wtsTenRBLabel",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsTenRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "10",
        NULL);



    wtsSweepsRB= XtVaCreateManagedWidget("wtsSweepsRB",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsTenRBLabel,
        XtNhorizDistance, 12,
        XtNvertDistance, 4,
        XtNbitmap, uncheckedRB,
        NULL);

    wtsSweepsRBLabel= XtVaCreateManagedWidget("wtsSweepsRBLabel",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsSweepsRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Sweeps",
        NULL);

    wtsEpochsRB= XtVaCreateManagedWidget("wtsEpochsRB",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsSweepsRBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, checkedRB,
        NULL);

    wtsEpochsRBLabel= XtVaCreateManagedWidget("wtsEpochsRBLabel",
        toggleWidgetClass, wtsForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, wtsEpochsRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Epochs",
        NULL);


    wtsViewport= XtVaCreateManagedWidget("wtsViewport",
        coreWidgetClass, wtsForm,
        XtNtranslations, XtParseTranslationTable(
            "<Configure>: aUpdateWtsDisplay() \n\
             <Expose>:    aPaintWtsDisplay()"),
        XtNtop, XtChainTop, 
        XtNleft, XtChainLeft,
        XtNright, XtChainRight,
        XtNvertDistance, 2,
        XtNborderWidth, 0,
        XtNfromVert, wtsClose,
        XtNwidth, DISPLAY_DEFAULT_WIDTH,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        NULL);

    XtAddCallback(wtsClose, XtNcallback, CloseWtsDisplay, NULL);
    XtAddCallback(wtsRedraw, XtNcallback, RedrawWtsDisplay, NULL);

    interval= 1L;
    epochs= animate= 1;
}


static void aWtsDispToggles(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (w == wtsUpdateCB || w == wtsUpdateCBLabel) {
        SetCheckBox(wtsUpdateCB, animate ^= 1);
        return;
    }

    if (w == wtsOneRB || w == wtsOneRBLabel) {
        if (interval == 1L) return;
        interval= 1L;
        SetRadioButtons(wtsOneRB, wtsFiveRB, wtsTenRB, 0);
    }

    else if (w == wtsFiveRB || w == wtsFiveRBLabel) {
        if (interval == 5L) return;
        interval= 5L;
        SetRadioButtons(wtsOneRB, wtsFiveRB, wtsTenRB, 1);
    }

    else if (w == wtsTenRB || w == wtsTenRBLabel) {
        if (interval == 10L) return;
        interval= 10L;
        SetRadioButtons(wtsOneRB, wtsFiveRB, wtsTenRB, 2);
    }

    else if (w == wtsSweepsRB || w == wtsSweepsRBLabel) {
        if (epochs == 0) return;
        SetRadioButtons(wtsSweepsRB, wtsEpochsRB, NULL, (epochs= 0));
    }

    else if (w == wtsEpochsRB || w == wtsEpochsRBLabel) {
        if (epochs == 1) return;
        SetRadioButtons(wtsSweepsRB, wtsEpochsRB, NULL, (epochs= 1));
    }

    SetWtsDispUpdate();
}


static void SetWtsDispUpdate(void)
{
    if (!epochs) next_update= sweep +interval;
    else  next_update= sweep + interval *tmax;
}


void AnimateWeightsDisplay(void)
{
    static long lastUpdate;
        
    if (!wtsPixmap) return;
    if (!animate) return;
    if (next_update > sweep && lastUpdate < sweep) return;
    aUpdateWtsDisplay(0, 0, 0, 0);
    SetWtsDispUpdate();
    lastUpdate= sweep;
}


void PopupWtsDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (init_config()) return;
    XtPopup(wtsShell, XtGrabNone);
    wtsWindow= XtWindow(wtsViewport);
    RedrawWtsDisplay(0, 0, 0);
}

extern int probe;

static void RedrawWtsDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{

    if (!gRunning) {
        int err;
        probe= 1;
        err= init_config();
        probe= 0;
        if (err || load_wts()) return;
    }

    aUpdateWtsDisplay(0, 0, 0, 0);
}


void aUpdateWtsDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (!XtIsRealized(wtsViewport)) return;

    XtVaGetValues(wtsViewport, XtNwidth,  &width, XtNheight, &height, NULL);

    if (wtsPixmap) XFreePixmap(appDisplay, wtsPixmap);
    wtsPixmap= 0;
    wtsPixmap= XCreatePixmap(appDisplay, wtsWindow, width, height, appDepth);

    if (wtsPixmap == 0) {
        CloseWtsDisplay(0, 0, 0);
        memerr("Offscreen wts");
        return;
    }
                
    XDefineCursor(appDisplay, wtsWindow, waitCursor);
    XFillRectangle(appDisplay, wtsPixmap, revGC, 0, 0, width, height);
    XDrawLine(appDisplay, wtsPixmap, appGC, 0, 0, width +10, 0);
    if (display_architecture(WEIGHTS)) CloseWtsDisplay(0, 0, 0);
    aPaintWtsDisplay(0, 0, 0, 0);
    XUndefineCursor(appDisplay, wtsWindow);
}
        

static void aPaintWtsDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (wtsPixmap == 0) return;
    XCopyArea(appDisplay, wtsPixmap, wtsWindow, appGC, 0, 0, 
              width, height, 0, 0);
}


static void CloseWtsDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (wtsPixmap) XFreePixmap(appDisplay, wtsPixmap);
    wtsPixmap= 0;
    XtPopdown(wtsShell);
}               


int WtsDispWidth(void)
{
    return (int)width;
}


int WtsDispHeight(void)
{
    return (int)height;
}




