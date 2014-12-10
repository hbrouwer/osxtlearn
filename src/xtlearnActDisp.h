#ifndef ACTDISP_H
#define ACTDISP_H

#include "general_X.h"

void InitActDisplay(XtAppContext appContext, Widget appWidget);
void PopupActDisplay(Widget w, XtPointer client_data, XtPointer call_data);
void ResetActDisplay(Widget w, XtPointer client_data, XtPointer call_data);
void aUpdateActDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
int ActDispWidth(void);
int ActDispHeight(void);


#endif
