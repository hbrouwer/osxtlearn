#ifndef ERRDISP_H
#define ERRDISP_H

#include "general_X.h"

void InitErrDisplay(XtAppContext appContext, Widget appWidget);
void PopupErrDisplay(Widget w, XtPointer client_data, XtPointer call_data);
void aUpdateErrDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
int AllocErrDisplay(void);
void PlotErrPoint(float *err);


#endif
