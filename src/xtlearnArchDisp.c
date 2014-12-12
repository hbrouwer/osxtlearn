#include "error.h"
#include "general_X.h"
#include "arch.h"
#include "xtlearn.h"
#include "xtlearnIface.h"
#include "xtlearnArchDisp.h"

extern int show_slabs, show_node_numbers, show_arrowheads, show_bias_node;
extern int arch_orientation;

extern Pixmap 
    checkedCB, uncheckedCB,
    checkedRB, uncheckedRB,
    horizIcon, vertIcon;

extern GC appGC, revGC;
extern Display *appDisplay;
extern int appDepth;
extern Cursor waitCursor;

Pixmap archPixmap; 
Widget archViewport;

static void aArchDispToggles(Widget w, XEvent *event, String *params, Cardinal *count);
static void aPaintArchDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
static void RedrawArchDisplay(Widget w, XtPointer client_data, XtPointer call_data);
static void CloseArchDisplay(Widget w, XtPointer client_data, XtPointer call_data);

static Window archWindow;

static Widget
    archShell,
    archForm,
    archClose,
    archRedraw,
    archShowLabel,
    archSlabsCB,
    archSlabsCBLabel,
    archBiasNodeCB,
    archBiasNodeCBLabel,
    archNodeNumsCB,
    archNodeNumsCBLabel,
    archArrowsCB,
    archArrowsCBLabel,
    archOrientLabel,
    archHorizRB,
    archHorizRBLabel,
    archVertRB,
    archVertRBLabel;

static Dimension width, height;

static XtActionsRec actions[]= {
    {"aArchDispToggles", aArchDispToggles },
    {"aPaintArchDisplay", aPaintArchDisplay },
    {"aUpdateArchDisplay", aUpdateArchDisplay }
};


void InitArchDisplay(XtAppContext appContext, Widget appWidget)
{
    XtTranslations toggleTrans;

    XtAppAddActions(appContext, actions, XtNumber(actions));

    toggleTrans= XtParseTranslationTable(
        "<Leave>:            reset()    \n\
         <BtnDown>,<BtnUp>: aArchDispToggles()");

    archShell= XtCreatePopupShell("Network Architecture",
        topLevelShellWidgetClass, appWidget, NULL, 0);

    archForm= XtVaCreateManagedWidget("archForm", 
        formWidgetClass, archShell,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        XtNdefaultDistance, 0,
        NULL);

    archClose= XtVaCreateManagedWidget("archClose", 
        commandWidgetClass, archForm, LOCK_GEOMETRY,
        XtNhorizDistance, 4,
        XtNvertDistance, 2,
        XtNlabel, " Close ",
        NULL);

    archRedraw= XtVaCreateManagedWidget("archRedraw", 
        commandWidgetClass, archForm, LOCK_GEOMETRY,
        XtNfromHoriz, archClose,
        XtNhorizDistance, 8,
        XtNvertDistance, 2,
        XtNlabel, " Redraw ",
        NULL);

    archShowLabel= XtVaCreateManagedWidget("archShowLabel", 
        labelWidgetClass, archForm, LOCK_GEOMETRY,
        XtNfromHoriz, archRedraw,
        XtNhorizDistance, 8,
        XtNvertDistance, 4,
        XtNlabel, "Show:",
        NULL);

    archSlabsCB= XtVaCreateManagedWidget("archSlabsCB",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archShowLabel,
        XtNvertDistance, 4,
        XtNbitmap, checkedCB,
        NULL);

    archSlabsCBLabel= XtVaCreateManagedWidget("archSlabsCBLabel",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archSlabsCB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Slabs",
        NULL);

    archNodeNumsCB= XtVaCreateManagedWidget("archNodeNumsCB",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archSlabsCBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, checkedCB,
        NULL);

    archNodeNumsCBLabel= XtVaCreateManagedWidget("archNodeNumsCBLabel",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archNodeNumsCB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Labels",
        NULL);

    archArrowsCB= XtVaCreateManagedWidget("archArrowsCB",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archNodeNumsCBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, checkedCB,
        NULL);

    archArrowsCBLabel= XtVaCreateManagedWidget("archArrowsCBLabel",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archArrowsCB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Arrows",
        NULL);

    archBiasNodeCB= XtVaCreateManagedWidget("archBiasNodeCB",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archArrowsCBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, uncheckedCB,
        NULL);

    archBiasNodeCBLabel= XtVaCreateManagedWidget("archBiasNodeCBLabel",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archBiasNodeCB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNlabel, "Bias",
        NULL);

    archOrientLabel= XtVaCreateManagedWidget("archOrientLabel",
        labelWidgetClass, archForm, LOCK_GEOMETRY,
        XtNfromHoriz, archBiasNodeCBLabel,
        XtNhorizDistance, 8,
        XtNvertDistance, 4,
        XtNlabel, "Orient:",
        NULL);

    archVertRB= XtVaCreateManagedWidget("archVertRB",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archOrientLabel,
        XtNvertDistance, 4,
        XtNbitmap, checkedRB,
        NULL);

    archVertRBLabel= XtVaCreateManagedWidget("archVertRBLabel",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archVertRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNbitmap, vertIcon,
        NULL);

    archHorizRB= XtVaCreateManagedWidget("archHorizRB",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archVertRBLabel,
        XtNhorizDistance, 6,
        XtNvertDistance, 4,
        XtNbitmap, uncheckedRB,
        NULL);

    archHorizRBLabel= XtVaCreateManagedWidget("archHorizRBLabel",
        toggleWidgetClass, archForm, LOCK_GEOMETRY,
        XtNtranslations, toggleTrans,
        XtNborderWidth, 0,
        XtNfromHoriz, archHorizRB,
        XtNvertDistance, 4,
        XtNinternalWidth, 0,
        XtNbitmap, horizIcon,
        NULL);

    archViewport= XtVaCreateManagedWidget("archViewport",
        coreWidgetClass, archForm,
        XtNtranslations, XtParseTranslationTable(
            "<Configure>: aUpdateArchDisplay() \n\
             <Expose>:    aPaintArchDisplay()"),
        XtNtop, XtChainTop, 
        XtNleft, XtChainLeft,
        XtNright, XtChainRight,
        XtNvertDistance, 2,
        XtNborderWidth, 0,
        XtNfromVert, archClose,
        XtNwidth, DISPLAY_DEFAULT_WIDTH,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        NULL);

    XtAddCallback(archClose, XtNcallback, CloseArchDisplay, NULL);
    XtAddCallback(archRedraw, XtNcallback, RedrawArchDisplay, NULL);

    show_bias_node= 0;
    show_slabs= show_node_numbers= show_arrowheads= 1;
    arch_orientation= 1;
}


static void aArchDispToggles(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (w == archVertRB || w == archVertRBLabel) {
        if (arch_orientation) return;
        SetRadioButtons(archHorizRB, archVertRB, NULL, arch_orientation= 1);
    }

    else if (w == archHorizRB  || w == archHorizRBLabel) {
        if (!arch_orientation) return;
        SetRadioButtons(archHorizRB, archVertRB, NULL, arch_orientation= 0);
    }
        
    else if (w == archSlabsCB || w == archSlabsCBLabel) {
        SetCheckBox(archSlabsCB, show_slabs ^= 1);
        if (show_slabs) SetCheckBox(archArrowsCB, show_arrowheads= 1);
    }
    else if (w == archNodeNumsCB || w == archNodeNumsCBLabel) 
        SetCheckBox(archNodeNumsCB, show_node_numbers ^= 1);
    else if (w == archArrowsCB || w == archArrowsCBLabel)
        SetCheckBox(archArrowsCB, show_arrowheads ^= 1);
    else if (w == archBiasNodeCB || w == archBiasNodeCBLabel) 
        SetCheckBox(archBiasNodeCB, show_bias_node ^= 1);
    else return;

    aUpdateArchDisplay(0, 0, 0, 0);
}


void PopupArchDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (init_config()) return;
    XtPopup(archShell, XtGrabNone);
    archWindow= XtWindow(archViewport);
    aUpdateArchDisplay(0, 0, 0, 0);
}


static void RedrawArchDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (init_config()) return;
    aUpdateArchDisplay(0, 0, 0, 0);
}


void aUpdateArchDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    if (!XtIsRealized(archViewport)) return;

    XtVaGetValues(archViewport, XtNwidth,  &width, XtNheight, &height, NULL);

    if (archPixmap) XFreePixmap(appDisplay, archPixmap);
    archPixmap= 0;
    archPixmap= XCreatePixmap(appDisplay, archWindow, width, height, appDepth);

    if (archPixmap == 0) {
        CloseArchDisplay(0, 0, 0);
        memerr("Offscreen arch");
        return;
    }

    XDefineCursor(appDisplay, archWindow, waitCursor);
    XFillRectangle(appDisplay, archPixmap, revGC, 0, 0, width, height);
    XDrawLine(appDisplay, archPixmap, appGC, 0, 0, width +10, 0);
    if (display_architecture(SPECIFICATION)) CloseArchDisplay(0, 0, 0);
    aPaintArchDisplay(0, 0, 0, 0);
    XUndefineCursor(appDisplay, archWindow);
}


static void aPaintArchDisplay(Widget w, XEvent *event, String *params, Cardinal *count)
{
    XCopyArea(appDisplay, archPixmap, archWindow, appGC, 0, 0, 
              width, height, 0, 0);
}


static void CloseArchDisplay(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (archPixmap) XFreePixmap(appDisplay, archPixmap);
    archPixmap= 0;
    XtPopdown(archShell);
}

int ArchDispWidth(void)
{
    return (int)width;
}


int ArchDispHeight(void)
{
    return (int)height;
}
