#ifndef ALERTBOX_H
#define ALERTBOX_H

#include "general_X.h"

void InitAlertBox(XtAppContext appContxt, Widget appWidget);
int AlertBox(char *message, int level, char *b1, char *b2, char *b3);


#endif
