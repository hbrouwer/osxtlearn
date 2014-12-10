#ifndef ARCHDISP_H
#define ARCHDISP_H

#include "general_X.h"

void InitArchDisplay(XtAppContext appContext, Widget appWidget);
void PopupArchDisplay(Widget w, XtPointer client_data, XtPointer call_data);
void aUpdateArchDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
int ArchDispWidth(void);
int ArchDispHeight(void);

#endif
