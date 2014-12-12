#include "general_X.h"
#include "xtlearnIface.h"
#include "xtlearnAlertBox.h"

#include "bitmaps/warningIcon"
#include "bitmaps/tlearnIcon"


static Widget
    dAlertShell,
    dAlertForm,  
    alertLabel,
    alertText,
    alertButton1,
    alertButton2,
    alertButton3;


static Pixmap
    warningIcon,
    tlearnIcon;

static XtAppContext appContext;
static int alertXPos, alertYPos, whichButton;

static void AlertPopdown(Widget w, XtPointer client_data, XtPointer call_data);


void InitAlertBox(XtAppContext appContxt, Widget appWidget)
{
    appContext= appContxt;
    warningIcon= XCreateBitmapFromData(XtDisplay(appWidget), 
        RootWindowOfScreen(XtScreen(appWidget)),
        warningIcon_bits, warningIcon_width, warningIcon_height);

    tlearnIcon= XCreateBitmapFromData(XtDisplay(appWidget), 
        RootWindowOfScreen(XtScreen(appWidget)),
        tlearnIcon_bits, tlearnIcon_width, tlearnIcon_height);

    dAlertShell= XtVaCreatePopupShell("OSXtlearn alert", 
        transientShellWidgetClass, appWidget, NULL);

    dAlertForm= XtVaCreateManagedWidget("dAlertForm",
        formWidgetClass, dAlertShell, 
        XtNdefaultDistance, 13, NULL);
    
    alertLabel= XtVaCreateManagedWidget(NULL,
        labelWidgetClass, dAlertForm, LOCK_GEOMETRY,
        XtNborderWidth, 0,
        XtNvertDistance, 15,
        XtNhorizDistance, 15,
        XtNwidth, 55,
        XtNheight, 55,
        XtNresize, FALSE,
        NULL);

    alertText= XtVaCreateManagedWidget("alertText",
        asciiTextWidgetClass, dAlertForm, 
/*LOCK_GEOMETRY,*/
        XtNwrap, XawtextWrapWord,
        XtNeditType, XawtextRead,
        XtNfromHoriz, alertLabel,
        XtNborderWidth, 0,
        XtNvertDistance, 15,
        XtNwidth, 380,
        XtNheight, 100,
        XtNsensitive, FALSE,
        XtNcursor, NULL,
        XtNdisplayCaret, 0,
        NULL);

    alertButton1= XtVaCreateManagedWidget("alertButton1",
        commandWidgetClass, dAlertForm,
/* LOCK_GEOMETRY,*/
 XtNtop, XtChainBottom, XtNbottom, XtChainBottom,
 XtNleft, XtChainLeft, XtNright, XtChainLeft,
        XtNfromVert, alertText,
        XtNvertDistance, 4,
        XtNhorizDistance, 390,
        NULL);

    alertButton2= XtVaCreateManagedWidget("alertButton2",
        commandWidgetClass, dAlertForm, 
/*LOCK_GEOMETRY,*/
 XtNtop, XtChainBottom, XtNbottom, XtChainBottom,
 XtNleft, XtChainLeft, XtNright, XtChainLeft,
        XtNfromVert, alertText,
        XtNvertDistance, 4,
        XtNhorizDistance, 300,
        NULL);

    alertButton3= XtVaCreateManagedWidget("alertButton3",
        commandWidgetClass, dAlertForm, 
/*LOCK_GEOMETRY,*/
 XtNtop, XtChainBottom, XtNbottom, XtChainBottom,
 XtNleft, XtChainLeft, XtNright, XtChainLeft,
        XtNfromVert, alertText,
        XtNvertDistance, 4,
        XtNhorizDistance, 150,
        NULL);

    XtAddCallback(alertButton1, XtNcallback, AlertPopdown, (XtPointer)dAlertShell);
    XtAddCallback(alertButton2, XtNcallback, AlertPopdown, (XtPointer)dAlertShell);
    XtAddCallback(alertButton3, XtNcallback, AlertPopdown, (XtPointer)dAlertShell);

    alertXPos= WidthOfScreen(XtScreen(appWidget))/2 -230;
    alertYPos= HeightOfScreen(XtScreen(appWidget))/4;
}


int AlertBox(char *message, int level, char *b1, char *b2, char *b3)
{
    static XEvent event;

    aResetMenuBar(0, 0, 0, 0);

    XtVaSetValues(dAlertShell, XtNx, alertXPos, XtNy, alertYPos, NULL);
    XtVaSetValues(alertText, XtNstring, message, NULL);


    if (level > 1) XtVaSetValues(alertLabel, XtNbitmap, warningIcon, NULL);
    else XtVaSetValues(alertLabel, XtNbitmap, tlearnIcon, NULL);

    XtVaSetValues(alertButton1, XtNlabel, (String)b1, NULL);

    if (b2) {
        XtVaSetValues(alertButton2, XtNlabel, (String)b2, NULL);
        XtManageChild(alertButton2);
    }
    else XtUnmanageChild(alertButton2);

    if (b3) {
        XtVaSetValues(alertButton3, XtNlabel, (String)b3, NULL);
        XtManageChild(alertButton3);
    }
    else XtUnmanageChild(alertButton3);

    XtPopup(dAlertShell, XtGrabExclusive);
    whichButton= 0;

    /* Alert has its own event loop because while we wait for the */
    /* user's response, we must handle any events that may occur. */

    while (whichButton == 0) {  /* loop until a button is pressed */
        XtAppNextEvent(appContext, &event);
        XtDispatchEvent(&event);
    }

    return whichButton;
}


static void AlertPopdown(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(dAlertShell);
    if (w == alertButton1) whichButton= 1;
    if (w == alertButton2) whichButton= 2;
    if (w == alertButton3) whichButton= 3;
}
