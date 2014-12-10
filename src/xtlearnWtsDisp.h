#ifndef WTSDISP_H
#define WTSDISP_H

#include "general_X.h"

void PaintWtsDisplay(void);
void InitWtsDisplay(XtAppContext appContext, Widget appWidget);
void PopupWtsDisplay(Widget w, XtPointer client_data, XtPointer call_data);
void aUpdateWtsDisplay(Widget w, XEvent *event, String *params, Cardinal *count);
void AnimateWeightsDisplay(void);
int WtsDispWidth(void);
int WtsDispHeight(void);


#endif
