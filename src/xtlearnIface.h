/*
	iface.h
*/

#include "general_X.h"  /* for Widget, XEvent, String and Cardinal types */

void aResetMenuBar(Widget w, XEvent *event, String *params, Cardinal *count);
void SetCheckBox(Widget w, int check);
void SetRadioButtons(Widget w0, Widget w1, Widget w2, int which);
void InitXInterface(int argc, char **argv);
void X_save_wts(Widget w, XtPointer client_data, XtPointer call_data);
void MainXEventLoop(void);
void XReportCondition(char *message, int level);

int NumStringOK(char *str, short no_dec_pt);
int LoadOptions(short run);
int SaveTrainingOptions(short run);
int SaveTestingOptions(void);
int DisplayOutputs(void);
int DisplayNodes(void);
void get_all_user_settings(void);

