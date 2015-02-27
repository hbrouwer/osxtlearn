/*
    xtlearnIface.c

    xtlearn X interface

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "general.h"
#include "error.h"
#include "general_X.h"
#include "xtlearnActDisp.h"
#include "xtlearnErrDisp.h"
#include "xtlearnWtsDisp.h"
#include "xtlearnArchDisp.h"
#include "xtlearnAlertBox.h"
#include "xtlearn.h"
#include "xtlearnIface.h"


#include "bitmaps/checkedCB"
#include "bitmaps/uncheckedCB"
#include "bitmaps/checkedRB"
#include "bitmaps/uncheckedRB"
#include "bitmaps/vertIcon"
#include "bitmaps/horizIcon"
#include "bitmaps/grayStipple"


    /* globals declared in xtlearn.c */

extern    float    rate;          /* learning rate */
extern    float    momentum;      /* momentum */
extern    float    weight_limit;  /* bound for random weight init */
extern    float    criterion;     /* exit program when rms error is less than this */
extern    float    init_bias;     /* possible offset for initial output biases */
extern    float    *zold;         /* (nt) inputs and activations at time t */

extern    long    nsweeps;    /* number of sweeps to run for */
extern    long    tmax;       /* maximum number of sweeps (given in .data) */
extern    long    umax;       /* update weights every umax sweeps */
extern    long    check;      /* output weights every "check" sweeps */
extern    long    sweep;      /* current sweep */
extern    long    tsweeps;    /* total sweeps to date */
extern    long    rms_report; /* output rms error every "report" sweeps */

extern    int    learning;    /* flag for learning */
extern    int    verify;
extern    int    probe;
extern    int    resume;

extern    int    nn;
extern    int    ni;         /* number of inputs */
extern    int    no;         /* number of outputs */
extern    int    *selects;   /* (nn+1) nodes selected for probe printout */
extern    int    *outputs;   /* (no) indices of output nodes */

extern    int    loadflag;    /* flag for loading initial weights from file */
extern    int    rflag;       /* flag for -x */
extern    int    seed;        /* seed for random() */
extern    int    fixedseed;   /* flag for random seed fixed by user */
extern    int    teacher;     /* flag for feeding back targets */
extern    int    randomly;    /* flag for presenting inputs in random order */
extern    int    ce;          /* flag for cross_entropy */
extern    int    copies, bptt;

extern    char    root[];      /* root filename; may change during testing */
extern    char    saveroot[]; /* root filename; NEVER CHANGES */
extern    char    loadfile[]; /* filename for weightfile to be read in */



extern int gAbort, gRunning;


extern Widget archViewport;
extern Widget actViewport;
extern Widget errViewport;
extern Widget wtsViewport;

static int gPause;


/* Make ANSI compilers really happy... */

static int OutputWindowHeader(void);
static int WriteToOutputFile(char *str);
static int WriteToOutputWindow(char *str);

static void AbortRun(Widget w, XtPointer client_data, XtPointer call_data);
static void AboutBox(Widget w, XtPointer client_data, XtPointer call_data);
static void CancelTestingDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void CancelTrainingDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void CheckTestOpts(void);
static void CheckTrainOpts(void);
static void DialogPopdown(Widget w, XtPointer client_data, XtPointer call_data);
static void DialogPopup(Widget w, XtPointer client_data, XtPointer call_data);
static void HistoryScroll(Widget w, XtPointer client_data, XtPointer call_data);
static void InitSetProjDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void InitTestingDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void InitTrainingDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void ManageTrainOptsWidgets(void);
static void OKSetProjDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void OKTestingDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void OKTrainingDialog(Widget w, XtPointer client_data, XtPointer call_data);
static void PlaceMenu(Widget w, XtPointer client_data, XtPointer call_data);
static void ProcessPendingXEvents(void);
static void ProcessXEvent(void);
static void Quit(Widget w, XtPointer client_data, XtPointer call_data);
static void RemoveTrainingOptions(Widget w, XtPointer client_data, XtPointer call_data);
static void SetAllTestOptsWidgets(void);
static void SetAllTrainOptsWidgets(void);
static void SetChangedTrainOptsWidgets(void);
static void SetRunWindowTitle(void);
static void SetValidDouble(Widget w, float *value, float max, short neg);
static void SetValidInt(Widget w, short *value, int max, int neg);
static void SetValidLong(Widget w, long *value);
static void SetWidgetDoubleText(Widget w, double value);
static void SetWidgetLongText(Widget w, long value, char blank);
static void StartSystem(Widget w, XtPointer client_data, XtPointer call_data);
static void ToggleBigTrainOpts(Widget w, XtPointer client_data, XtPointer call_data);
static void UpdateStatusSweeps(void);
static void UpdateStatusText(char *state);
static void UpdateStatusWindowButtons(void);
static void aMaintainMenus(Widget w, XEvent *event, String *params, Cardinal *c);
static void aPressOK(Widget w, XEvent *event, String *params, Cardinal *count);
static void aSetFocus(Widget w, XEvent *event, String *params, Cardinal *count);
static void aSpecialDeletePrev(Widget w, XEvent *event, String *params, Cardinal *count);
static void aSpecialInsertChar(Widget w, XEvent *event, String *params, Cardinal *count);
static void aTestOptsDlgToggles(Widget w, XEvent *event, String *params, Cardinal *c);
static void aTrainOptsDlgToggles(Widget w, XEvent *event, String *params, Cardinal *c);
static void aUnSelect(Widget w, XEvent *event, String *params, Cardinal *count);


static char buf[256];  /* utility buffer used throughout */
static int checkTrainOpts, checkTestOpts, checkSetProj;
static FILE *output_file;

/* X Application globals */

XtAppContext appContext;

Display *appDisplay;

Cursor waitCursor;

GC appGC, revGC;

int appDepth;

Widget appWidget;

Window appWindow;


/* Main Form Widgets */

static Widget 
    mainForm,
    statusBar,
    projLabel,
    projName,
    sweepCountLabel,
    sweepCount,
    weightsButton,
    abortButton,
    infoArea, 
    mFileBar,  mFileShell,  mFileBox,  mFilePane[5],      /* menu widgets */
    mTrainBar, mTrainShell, mTrainBox, mTrainPane[5],
    mTestBar,  mTestShell,  mTestBox,  mTestPane[5],
    mDispBar,  mDispShell,  mDispBox,  mDispPane[5];

/* Training Options Dialog Widgets */

static Widget 
    trainOptsShell,
    trainOptsForm,
    sweepsLabel,
    momentumLabel,
    rateLabel,
    stopLabel,
    updateLabel,
    offsetLabel,
    sweeps1, sweeps2, sweeps3, copies1,
    runNameText,
    weightsText,
    sweepsText,
    momentumText,
    rateText,
    reportText,
    checkText,
    stopText,
    updateText,
    offsetText,
    bpttText,
    seedText,
    seedWithRB,
    seedRandRB,
    trainSequRB,
    trainRandRB,
    rmsErrorRB,
    c_eErrorRB,
    bothErrorRB,
    logErrorCB, 
    logWeightsCB,
    loadWeightsCB,
    bpttCB,
    resetCB,
    teachForceCB,
    seedWithRBLabel,
    seedRandRBLabel,
    trainSequRBLabel,
    trainRandRBLabel,
    rmsErrorRBLabel,
    c_eErrorRBLabel,
    bothErrorRBLabel,
    logErrorCBLabel, 
    logWeightsCBLabel,
    loadWeightsCBLabel,
    bpttCBLabel,
    resetCBLabel,
    teachForceCBLabel,
    trainButtonGap,
    trainMore,
    removeButton,
    lastButton,
    nextButton,
    trainCancel,
    trainOK;

/* Testing Options Dialog Widgets */

static Widget
    testOptsShell,
    testOptsForm,
    testOptsTitle,
    testSweepsLabel, 
    testSweepsText,
    testWeightsLabel, 
    mostRecentLabel,
    alwaysUseText,
    testDataLabel, 
    trainingSetLabel, 
    novelDataText,
    outputFileText, 
    mostRecentWtsRB, 
    alwaysUseWtsRB, 
    trainingSetRB, 
    novelDataRB, 
    autoSweepsRB, 
    testSweepsRB,
    outputToScrCB,
    outputToFileCB,

    mostRecentWtsRBLabel, 
    alwaysUseWtsRBLabel, 
    trainingSetRBLabel, 
    novelDataRBLabel, 
    autoSweepsRBLabel, 

    outputToScrCBLabel,
    outputToFileCBLabel,

    testCalcErrorCB,
    testCalcErrorCBLabel,
    testLogErrorCB,
    testLogErrorCBLabel,

    testCancel, 
    testOK;


/* Set Project Dialog Widgets */

static Widget
    setProjShell,
    setProjForm,
    setProjLabel,
    setProjText,
    setProjCancel,
    setProjOK;

/* Output Window Widgets */

static Widget 
    outputShell, 
    outputForm, 
    outputText, 
    outputClose;



/*******************/


TrainingOptions 
    TrainOpts,      /* main storage of TrainOptions */
    SaveTrainOpts,  /* copy used to restore TrainOpts when 'Cancel' is pressed */
    recentOptions,  /* holds current TrainOpts if 'Prev' is pressed */
    tmpState;       /* used for comparison to see if user attempted a change */

TestingOptions TestOpts, SaveTestOpts;
short gLastRun, runNumber;


Pixmap
    checkedCB, uncheckedCB,  /* Check Box Bitmaps */
    checkedRB, uncheckedRB,  /* Radio Button Bitmaps */
    horizIcon, vertIcon,/* Arch Orientation Arrow Bitmaps (move to arch file? )*/
    grayStipple;



/* 
   These are the actions assigned to various widgets and menus that
   make the user interface work the way it does.  They are actions
   in the strict sense: they have the required arguments and are
   only called by X in response to user actions specified by the
   translation table.  Some change members of the TrainOpts and
   TestOpts arrays and then call one of the generic Set... functions
   to change the appearance of the widgets if necessary.
*/


static void aMaintainMenus(Widget w, XEvent *event, String *params, Cardinal *c)
{
    int i;

    gPause= 1;

    for (i= 0; i < 4; i++) XtSetSensitive(mTrainPane[i], 0);
    for (i= 0; i < 4; i++) XtSetSensitive(mTestPane[i], 0);

    if (!*root) {
        for (i= 0; i < 5; i++) XtSetSensitive(mDispPane[i], 0);
        return;
    }

    for (i= 0; i < 5; i++) XtSetSensitive(mDispPane[i], 1);

    if (gRunning) {
        XtSetSensitive(mTrainPane[0], 1);
        XtSetSensitive(mTestPane[0], 1);
        XtSetSensitive(mTrainPane[3], 1);
        XtSetSensitive(mTestPane[3], 1);
    }
    else {
        for (i= 0; i < 3; i++) XtSetSensitive(mTrainPane[i], 1);
        for (i= 0; i < 3; i++) XtSetSensitive(mTestPane[i], 1);
    }


}



    


static void aTrainOptsDlgToggles(Widget w, XEvent *event, String *params, Cardinal *c)
{
    if (w == seedWithRB || w == seedWithRBLabel) {
        if (TrainOpts.fixedSeed) return;
        SetRadioButtons(seedRandRB, seedWithRB, NULL, TrainOpts.fixedSeed= 1);
    }

    else if (w == seedRandRB || w == seedRandRBLabel) {
        if (!TrainOpts.fixedSeed) return; 
        SetRadioButtons(seedRandRB, seedWithRB, NULL, TrainOpts.fixedSeed= 0);
    }

    else if (w == trainRandRB || w == trainRandRBLabel) {
        if (TrainOpts.randPatterns) return; 
        SetRadioButtons(trainSequRB, trainRandRB, NULL, TrainOpts.randPatterns= 1);
    }

    else if (w == trainSequRB || w == trainSequRBLabel) {
        if (!TrainOpts.randPatterns) return;
        SetRadioButtons(trainSequRB, trainRandRB, NULL, TrainOpts.randPatterns= 0);
    }

    else if (w == rmsErrorRB || w == rmsErrorRBLabel) {
        if (TrainOpts.crossEntropyOpt == 0) return;
        SetRadioButtons(rmsErrorRB, bothErrorRB, c_eErrorRB, TrainOpts.crossEntropyOpt= 0);
    }

    else if (w == bothErrorRB || w == bothErrorRBLabel) {
        if (TrainOpts.crossEntropyOpt == 1) return;
        SetRadioButtons(rmsErrorRB, bothErrorRB, c_eErrorRB, TrainOpts.crossEntropyOpt= 1);
    }

    else if (w == c_eErrorRB || w == c_eErrorRBLabel) {
        if (TrainOpts.crossEntropyOpt == 2) return;
        SetRadioButtons(rmsErrorRB, bothErrorRB, c_eErrorRB, TrainOpts.crossEntropyOpt= 2);
    }

    else if (w == logErrorCB || w == logErrorCBLabel)
        SetCheckBox(logErrorCB, TrainOpts.writeError ^= 1);
    else if (w == logWeightsCB || w == logWeightsCBLabel)
        SetCheckBox(logWeightsCB, TrainOpts.dumpWeights ^= 1); 
    else if (w == loadWeightsCB || w == loadWeightsCBLabel)
        SetCheckBox(loadWeightsCB, TrainOpts.loadWeights ^= 1); 
    else if (w == bpttCB || w == bpttCBLabel)
        SetCheckBox(bpttCB, TrainOpts.bpttFlag ^= 1); 
    else if (w == resetCB || w == resetCBLabel)
        SetCheckBox(resetCB, TrainOpts.useResetFile ^= 1); 
    else if (w == teachForceCB || w == teachForceCBLabel)
        SetCheckBox(teachForceCB, TrainOpts.teacherForcing ^= 1); 
}


static void aTestOptsDlgToggles(Widget w, XEvent *event, String *params, Cardinal *c)
{

    if (w == alwaysUseWtsRB || w == alwaysUseWtsRBLabel) {
        if (TestOpts.mostRecentWts == 0) return;
        SetRadioButtons(alwaysUseWtsRB, mostRecentWtsRB, NULL, TestOpts.mostRecentWts= 0);
    }

    else if (w == mostRecentWtsRB || w == mostRecentWtsRBLabel) {
        if (TestOpts.mostRecentWts == 1) return;
        SetRadioButtons(alwaysUseWtsRB, mostRecentWtsRB, NULL, TestOpts.mostRecentWts= 1);
    }

    else if (w == trainingSetRB || w == trainingSetRBLabel) {
        if (TestOpts.novelData == 0) return;
        SetRadioButtons(trainingSetRB, novelDataRB, NULL, TestOpts.novelData= 0);
    }

    else if (w == novelDataRB || w == novelDataRBLabel) {
        if (TestOpts.novelData == 1) return;
        SetRadioButtons(trainingSetRB, novelDataRB, NULL, TestOpts.novelData= 1);
    }

    else if (w == testSweepsRB) {
        if (TestOpts.autoSweeps == 0) return;
        SetRadioButtons(testSweepsRB, autoSweepsRB, NULL, TestOpts.autoSweeps= 0);
    }

    else if (w == autoSweepsRB || w == autoSweepsRBLabel) {
        if (TestOpts.autoSweeps == 1) return;
        SetRadioButtons(testSweepsRB, autoSweepsRB, NULL, TestOpts.autoSweeps= 1);
    }

    else if (w == outputToFileCB || w == outputToFileCBLabel)
        SetCheckBox(outputToFileCB, TestOpts.outputDestFile ^= 1);
    else if (w == outputToScrCB || w == outputToScrCBLabel)
        SetCheckBox(outputToScrCB, TestOpts.outputDestScr  ^= 1);

    else if (w == testCalcErrorCB || w == testCalcErrorCBLabel)
        SetCheckBox(testCalcErrorCB, TestOpts.calculateError  ^= 1);
    else if (w == testLogErrorCB || w == testLogErrorCBLabel)
        SetCheckBox(testLogErrorCB, TestOpts.writeError  ^= 1);
}


static void aPressOK(Widget w, XEvent *event, String *params, Cardinal *count)
{
    Boolean sensitive;

    if (checkTrainOpts) w= trainOK;
    else if (checkTestOpts) w= testOK;
    else if (checkSetProj) w= setProjOK;
    else return;

    XtVaGetValues(w, XtNsensitive, &sensitive, NULL);
    if (!sensitive) return;

    XtCallActionProc(w, "set", event, params, *count);
    XtCallActionProc(w, "notify", event, params, *count);
    XtCallActionProc(w, "reset", event, params, *count);

}


void aResetMenuBar(Widget w, XEvent *event, String *params, Cardinal *count)
{
    XtPopdown(mFileShell);
    XtPopdown(mTrainShell);
    XtPopdown(mTestShell);
    XtPopdown(mDispShell);

    XtCallActionProc(mFileBar, "reset", 0, 0, 0);
    XtCallActionProc(mTrainBar, "reset", 0, 0, 0);
    XtCallActionProc(mTestBar, "reset", 0, 0, 0);
    XtCallActionProc(mDispBar, "reset", 0, 0, 0);
    gPause= 0;
}


static void aSpecialInsertChar(Widget w, XEvent *event, String *params, Cardinal *count)
{
    long start, end;

    XawTextGetSelectionPos(w, &start, &end);
    if (start < end) {
        XtCallActionProc(w, "delete-selection", event, params, *count);
        XawTextSetInsertionPoint(w, start);
    }

    XtCallActionProc(w, "insert-char", event, params, *count);
}


static void aSpecialDeletePrev(Widget w, XEvent *event, String *params, Cardinal *count)
{
    long start, end;

    XawTextGetSelectionPos(w, &start, &end);
    if (start < end) {
        XtCallActionProc(w, "delete-selection", event, params, *count);
        XawTextSetInsertionPoint(w, start);
    }

    else XtCallActionProc(w, "delete-previous-character", event, params, *count);
}


static void aSetFocus(Widget w, XEvent *event, String *params, Cardinal *count)
{
    XtSetKeyboardFocus(XtParent(w), w);
}


static void aUnSelect(Widget w, XEvent *event, String *params, Cardinal *count)
{
    XawTextUnsetSelection(w);   
}



/* Callbacks */

static void PlaceMenu(Widget w, XtPointer client_data, XtPointer call_data)
{
    Position x, y;
    Dimension height;

    XtTranslateCoords((Widget)client_data, (Position)0, (Position)0, &x, &y);
    XtVaGetValues((Widget)client_data, XtNheight, &height, NULL);
    XtVaSetValues(w, XtNx, x, XtNy, y+ height, NULL);
}


static void DialogPopup(Widget w, XtPointer client_data, XtPointer call_data)
{
    Position x, y;

    XtTranslateCoords(infoArea, (Position)0, (Position)0, &x, &y);
    XtVaSetValues((Widget)client_data, XtNx, x+40, XtNy, y+25, NULL);

    XtPopup((Widget)client_data, XtGrabNone);
    XtSetSensitive(mainForm, 0);
}


static void DialogPopdown(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtSetSensitive(mainForm, 1);
    XtPopdown((Widget)client_data);
    checkTestOpts= checkTrainOpts= checkSetProj= 0;
}


static void InitTrainingDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    SaveTrainOpts= TrainOpts;
    runNumber= gLastRun +1;
    SetAllTrainOptsWidgets();
    checkTrainOpts= 1;

    XawTextSetSelection(sweepsText, 0, 12);
    XtSetKeyboardFocus(XtParent(sweepsText), sweepsText);
}


static void InitTestingDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    SaveTestOpts= TestOpts;
    SetAllTestOptsWidgets();
    checkTestOpts= 1;

    XawTextSetSelection(alwaysUseText, 0, 40);
    XtSetKeyboardFocus(XtParent(alwaysUseText), alwaysUseText);
}


static void InitSetProjDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    checkSetProj= 1;

    XtVaSetValues(setProjText, XtNstring, (String)saveroot, NULL);

    XawTextSetSelection(setProjText, 0, 60);
    XtSetKeyboardFocus(XtParent(setProjText), setProjText);
}


static void CancelTrainingDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    TrainOpts= SaveTrainOpts;
    DialogPopdown(w, client_data, 0);
}


static void CancelTestingDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    TestOpts= SaveTestOpts;
    DialogPopdown(w, client_data, 0);
}


static void OKTestingDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    String text;
    FILE *fp;
    int len;

    XtVaGetValues(alwaysUseText, XtNstring, &text, NULL);
    strcpy(TestOpts.weightsFile, text);

    XtVaGetValues(novelDataText, XtNstring, &text, NULL);
    strcpy(TestOpts.novelFile, text);

    XtVaGetValues(outputFileText, XtNstring, &text, NULL);
    strcpy(TestOpts.outputFile, text);

    if (!TestOpts.mostRecentWts) {
        if (*TestOpts.weightsFile) {
            fp= fopen(TestOpts.weightsFile, "r");
            if (fp) fclose(fp);
            else {
                report_condition("Weights file could not be found.", 2);
                return;
            }
        }
        else TestOpts.mostRecentWts= 1;
    }

    if (*TestOpts.novelFile) {
        len= strlen(TestOpts.novelFile);
        while (TestOpts.novelFile[len -1] == ' ')
            TestOpts.novelFile[--len]= 0; /* truncate trailing spaces */

        if (!strcmp((TestOpts.novelFile +len -5), ".data"))
            TestOpts.novelFile[len -5]= 0; /* truncate '.data' suffix */
    }

    if (TestOpts.novelData) {
        if (*TestOpts.novelFile) {
            sprintf(buf, "%s.data", TestOpts.novelFile);
            fp= fopen(buf, "r");
            if (fp) fclose(fp);
            else {
                report_condition("Novel data file could not be found.  (Note that a matching novel teach file must also exist for error to be calculated.)", 2);
                return;
            }
        }
        else TestOpts.novelData= 0;
    }

    if (TestOpts.outputDestFile && !*TestOpts.outputFile)
        TestOpts.outputDestFile= 0;

    if (!TestOpts.outputDestFile && !TestOpts.outputDestScr)
        TestOpts.outputDestScr= 1;


    if (TestOpts.calculateError && !SaveTestOpts.calculateError)
        ResetActDisplay(0, 0, 0);  /* rare case, but must be done */


    DialogPopdown(w, client_data, 0);
    SaveTestingOptions();
}


static void OKTrainingDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    String text;

    XtVaGetValues(weightsText, XtNstring, &text, NULL);
    strcpy(TrainOpts.weightsFile, text);

    if (TrainOpts.loadWeights) {
        if (*TrainOpts.weightsFile) {
            FILE *fp= fopen(TrainOpts.weightsFile, "r");
            if (fp) fclose(fp);
            else {
                report_condition("Weights file does not exist.", 2);
                return;
            }
        }
        else TrainOpts.loadWeights= 0;
    }

    if (SaveTrainOpts.bpttFlag != TrainOpts.bpttFlag || 
        SaveTrainOpts.bpttCopies != TrainOpts.bpttCopies) {
        SilenceErrors(1);
        if (!gRunning && !set_up_simulation()) {
            aUpdateArchDisplay(0, 0, 0, 0);
            aUpdateWtsDisplay(0, 0, 0, 0);
            aUpdateActDisplay(0, 0, 0, 0);
        }
        SilenceErrors(0);
    }

    DialogPopdown(w, client_data, 0);
}


static void OKSetProjDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
    String text;

    XtVaGetValues(setProjText, XtNstring, &text, NULL);
    if (strlen((char *)text) > 128) *((char *)text +127)= 0;
    strcpy(root, text);
    if (LoadOptions(0)) {
        strcpy(root, saveroot);
        return;
    }

    strcpy(saveroot, root);  /* permanent copy */
    XtVaSetValues(projName, XtNlabel, text, NULL);
    ManageTrainOptsWidgets(); /* For bigDialog toggle */

    SilenceErrors(1);
    if (!gRunning && !init_config()) {
        aUpdateArchDisplay(0, 0, 0, 0);
        aUpdateWtsDisplay(0, 0, 0, 0);
        aUpdateActDisplay(0, 0, 0, 0);
    }

    SilenceErrors(0);

    DialogPopdown(w, client_data, 0);
}


static void HistoryScroll(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (w == nextButton) {
        if (runNumber > gLastRun) return;
        tmpState= TrainOpts;
        if (++runNumber > gLastRun) TrainOpts= recentOptions;
        else if (LoadOptions(runNumber)) runNumber--;
    }
    else {
        if (runNumber <= 1) return;
        tmpState= TrainOpts;
        if (runNumber == gLastRun +1) recentOptions= TrainOpts;
        if (LoadOptions(--runNumber)) runNumber++; 
     }
        
    SetChangedTrainOptsWidgets();
}


static void RemoveTrainingOptions(Widget w, XtPointer client_data, XtPointer call_data)
{
    TrainingOptions tmpBuf;
    FILE *prjfile, *newfile;
    int i;

    sprintf(buf, "%s.prj", saveroot);
    prjfile= fopen(buf, "r+");
    newfile= fopen("temp.xtlearn.prj", "w+");

    fwrite(&TestOpts, sizeof(TestOpts), 1, newfile);
    fseek(prjfile, sizeof(TestOpts), SEEK_SET);

    for (i= 1; i< runNumber; i++) {    
        fread(&tmpBuf, sizeof(TrainOpts), 1, prjfile);
        fwrite(&tmpBuf, sizeof(TrainOpts), 1, newfile);
    }

    fseek(prjfile, sizeof(TrainOpts), SEEK_CUR);

    while (fread(&tmpBuf, sizeof(TrainOpts), 1, prjfile))
        fwrite(&tmpBuf, sizeof(TrainOpts), 1, newfile);

    fclose(prjfile);
    fclose(newfile);

    remove(buf);
    rename("temp.xtlearn.prj", buf);

    gLastRun--;
    if (runNumber > gLastRun) TrainOpts= recentOptions;
    else LoadOptions(runNumber);
    
    tmpState= TrainOpts;

    SetAllTrainOptsWidgets();

}


static void CloseOutputShell(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(outputShell);
}


/*
   These are the generic Set... functions.  All they do is make sure
   the apperance of the widgets is in sync with the values in the
   TrainOpts and TestOpts arrays.  They are usually called all at once 
   by the SetAllTrainOptsWidgets function.  The TrainOpts and TestOpts
   arrays are the final word for all parameters.  Some widgets
   alter these values real-time, while others require an added
   preocessing step.  This is unfortunate, but there is no single 
   solution that fits all situations.
*/


void SetCheckBox(Widget w, int check)
{
    XtVaSetValues(w, XtNbitmap, check ? checkedCB : uncheckedCB, NULL);
    ProcessPendingXEvents();  /* make sure the box gets drawn */
}


void SetRadioButtons(Widget w0, Widget w1, Widget w2, int which)
{
    XtVaSetValues(w0, XtNbitmap, which == 0 ? checkedRB : uncheckedRB, NULL);
    XtVaSetValues(w1, XtNbitmap, which == 1 ? checkedRB : uncheckedRB, NULL);
    if (w2) XtVaSetValues(w2, XtNbitmap, which == 2 ? checkedRB : uncheckedRB, NULL);
    ProcessPendingXEvents();  /* make sure the buttons gets drawn */
}


static void SetWidgetLongText(Widget w, long value, char blank)
{
   *buf= 0; 
   if (value != 0L || !blank) sprintf(buf, "%ld", value);
   XtVaSetValues(w, XtNstring, (String)buf, NULL);
}


static void SetWidgetDoubleText(Widget w, double value)
{
    sprintf(buf, "%.4f", (double)value);
    XtVaSetValues(w, XtNstring, (String)buf, NULL);
}


static void SetAllTrainOptsWidgets(void)
{
    SetWidgetLongText(sweepsText, TrainOpts.sweeps, 1);
    SetWidgetLongText(seedText, TrainOpts.userSeed, 0);
    SetWidgetDoubleText(rateText, TrainOpts.learnRate);
    SetWidgetDoubleText(momentumText, TrainOpts.momentum);

    SetRunWindowTitle();

    SetRadioButtons(seedRandRB, seedWithRB, NULL, TrainOpts.fixedSeed);
    SetRadioButtons(trainSequRB, trainRandRB, NULL, TrainOpts.randPatterns);
    SetRadioButtons(rmsErrorRB, bothErrorRB, c_eErrorRB, TrainOpts.crossEntropyOpt);

    SetCheckBox(logErrorCB, TrainOpts.writeError);
    SetCheckBox(logWeightsCB, TrainOpts.dumpWeights);
    SetCheckBox(loadWeightsCB, TrainOpts.loadWeights);
    SetCheckBox(bpttCB, TrainOpts.bpttFlag);
    SetCheckBox(resetCB, TrainOpts.useResetFile);
    SetCheckBox(teachForceCB, TrainOpts.teacherForcing);

    SetWidgetLongText(reportText, TrainOpts.errorInterval, 1);
    SetWidgetLongText(checkText, TrainOpts.dumpInterval, 1);
    SetWidgetLongText(bpttText, TrainOpts.bpttCopies, 1);
    SetWidgetLongText(updateText, TrainOpts.updateInterval, 0);
    SetWidgetDoubleText(offsetText, TrainOpts.initialBias);
    SetWidgetDoubleText(stopText, TrainOpts.errCriterion);
    XtVaSetValues(weightsText, XtNstring, TrainOpts.weightsFile, NULL);
}


    /* This monstrosity is the same idea as SetAllTrainOpts but it is  */
    /* used when the dialog is actually visible.  The purpose of it    */
    /* is to avoid unnecessary updates that flash and look tacky.  For */
    /* it to work, the global struct tmpState needs to be current.     */

static void SetChangedTrainOptsWidgets(void)
{
    SetRunWindowTitle();

    if (!memcmp((void *)&TrainOpts, (void *)&tmpState, sizeof(TrainOpts) -40)) return;

    if (TrainOpts.sweeps != tmpState.sweeps) SetWidgetLongText(sweepsText, TrainOpts.sweeps, 1);
    if (TrainOpts.userSeed != tmpState.userSeed) SetWidgetLongText(seedText, TrainOpts.userSeed, 0);
    if (TrainOpts.learnRate != tmpState.learnRate) SetWidgetDoubleText(rateText, TrainOpts.learnRate);
    if (TrainOpts.momentum != tmpState.momentum) SetWidgetDoubleText(momentumText, TrainOpts.momentum);

    if (TrainOpts.fixedSeed != tmpState.fixedSeed) SetRadioButtons(seedRandRB, seedWithRB, NULL, TrainOpts.fixedSeed);
    if (TrainOpts.randPatterns != tmpState.randPatterns) SetRadioButtons(trainSequRB, trainRandRB, NULL, TrainOpts.randPatterns);
    if (TrainOpts.crossEntropyOpt != tmpState.crossEntropyOpt) SetRadioButtons(rmsErrorRB, bothErrorRB, c_eErrorRB, TrainOpts.crossEntropyOpt);

    if (TrainOpts.writeError != tmpState.writeError) SetCheckBox(logErrorCB, TrainOpts.writeError);
    if (TrainOpts.dumpWeights != tmpState.dumpWeights) SetCheckBox(logWeightsCB, TrainOpts.dumpWeights);
    if (TrainOpts.loadWeights != tmpState.loadWeights) SetCheckBox(loadWeightsCB, TrainOpts.loadWeights);
    if (TrainOpts.bpttFlag != tmpState.bpttFlag) SetCheckBox(bpttCB, TrainOpts.bpttFlag);
    if (TrainOpts.useResetFile != tmpState.useResetFile) SetCheckBox(resetCB, TrainOpts.useResetFile);
    if (TrainOpts.teacherForcing != tmpState.teacherForcing) SetCheckBox(teachForceCB, TrainOpts.teacherForcing);

    if (TrainOpts.errorInterval != tmpState.errorInterval) SetWidgetLongText(reportText, TrainOpts.errorInterval, 1);
    if (TrainOpts.dumpInterval != tmpState.dumpInterval) SetWidgetLongText(checkText, TrainOpts.dumpInterval, 1);
    if (TrainOpts.bpttCopies != tmpState.bpttCopies) SetWidgetLongText(bpttText, TrainOpts.bpttCopies, 1);
    if (TrainOpts.updateInterval != tmpState.updateInterval) SetWidgetLongText(updateText, TrainOpts.updateInterval, 0);
    if (TrainOpts.initialBias != tmpState.initialBias) SetWidgetDoubleText(offsetText, TrainOpts.initialBias);
    if (TrainOpts.errCriterion != tmpState.errCriterion) SetWidgetDoubleText(stopText, TrainOpts.errCriterion);
    if (strcmp(TrainOpts.weightsFile, tmpState.weightsFile)) XtVaSetValues(weightsText, XtNstring, TrainOpts.weightsFile, NULL);
}


static void SetAllTestOptsWidgets(void)
{
    SetRadioButtons(alwaysUseWtsRB, mostRecentWtsRB, NULL, TestOpts.mostRecentWts);
    SetRadioButtons(trainingSetRB, novelDataRB, NULL, TestOpts.novelData);
    SetRadioButtons(testSweepsRB, autoSweepsRB, NULL, TestOpts.autoSweeps);

    SetCheckBox(outputToFileCB, TestOpts.outputDestFile);
    SetCheckBox(outputToScrCB, TestOpts.outputDestScr);
    SetCheckBox(testCalcErrorCB, TestOpts.calculateError);
    SetCheckBox(testLogErrorCB, TestOpts.writeError);

    if (TestOpts.mostRecentFile[0]) {
        sprintf(buf, "[%s]", TestOpts.mostRecentFile);
        XtVaSetValues(mostRecentLabel, XtNlabel, (String)buf, NULL);
    }
    else XtVaSetValues(mostRecentLabel, XtNlabel, "<< not available >>", NULL);
    
    XtVaSetValues(alwaysUseText, XtNstring, TestOpts.weightsFile, NULL);
    
    sprintf(buf, "[%s.data]", saveroot);
    XtVaSetValues(trainingSetLabel, XtNlabel, (String)buf, NULL);
    *buf= 0;
    if (*TestOpts.novelFile) sprintf(buf, "%s.data", TestOpts.novelFile);
    XtVaSetValues(novelDataText, XtNstring, buf, NULL);
    XtVaSetValues(outputFileText, XtNstring, TestOpts.outputFile, NULL);
    SetWidgetLongText(testSweepsText, TestOpts.sweeps, 1);   
}

    /* Copy all user-set parameters to the analogous tlearn parameters */

void get_all_user_settings(void)
{
    check= (TrainOpts.dumpWeights) ? TrainOpts.dumpInterval : 0;
    momentum=    TrainOpts.momentum;
    rate=        TrainOpts.learnRate;
    nsweeps=     TrainOpts.sweeps;
    rflag=       TrainOpts.useResetFile;
    rms_report=  (TrainOpts.writeError && learning) ? TrainOpts.errorInterval : 0;
    criterion=   TrainOpts.errCriterion;
    randomly=    TrainOpts.randPatterns && learning;
    fixedseed=   TrainOpts.fixedSeed;
    seed=        TrainOpts.userSeed;
    umax=        TrainOpts.updateInterval;
    init_bias=   TrainOpts.initialBias;
    ce=          TrainOpts.crossEntropyOpt;
    teacher=     TrainOpts.teacherForcing;
    bptt=        TrainOpts.bpttFlag;
    copies=      TrainOpts.bpttCopies;

    loadflag= 1;      /* Determine which weights file to load */
        
    if (resume || ((probe || verify) && TestOpts.mostRecentWts))
        strcpy(loadfile, TestOpts.mostRecentFile);
    else if (learning && TrainOpts.loadWeights)
         strcpy(loadfile, TrainOpts.weightsFile);
    else if (probe || verify)
        strcpy(loadfile, TestOpts.weightsFile);
    else loadflag= 0;
        
}


    /* Copy all tlearn parameters to the analogous user-set parameters.  */
    /* Only used once to get command line settings.  If there are ANY
       command line settings, the project file is not used for the
       initial settings.
    */

static void set_all_user_settings(void)
{
    TrainOpts.dumpWeights= TrainOpts.dumpInterval= check;
    TrainOpts.momentum= momentum;
    TrainOpts.learnRate= rate;
    TrainOpts.sweeps= nsweeps;
    TrainOpts.useResetFile= rflag;
    TrainOpts.writeError= TrainOpts.errorInterval= rms_report;
    TrainOpts.errCriterion= criterion;
    TrainOpts.randPatterns= randomly;
    TrainOpts.fixedSeed= fixedseed;
    TrainOpts.userSeed= seed;
    TrainOpts.updateInterval= umax;
    TrainOpts.initialBias= init_bias;
    TrainOpts.crossEntropyOpt= ce;
    TrainOpts.teacherForcing= teacher;
    TrainOpts.loadWeights= loadflag;
    strcpy(TrainOpts.weightsFile, loadfile);
}



/* These are the SetValid... utility functions.  They make sure that the
   value in the widget is valid, and if it is, set the TrainOpts or TestOpts
   member to the value.  If the value in the widget is invalid, they beep
   and put the most recent valid value back (that is, the value stored in
   the TrainOpts or TestOpts structure).  These functions are usually called
   by the CheckTrainOpts or CheckTestOpts.  CheckTrainOpts and CheckTestOpts
   are called EVERY PASS THROUGHT THE EVENT LOOP  while their respective 
   dialogs are up.  This may seem excessive, but it is the simplest method
   that doesn't (seem to) cause compatibility problems.
*/

static void SetValidInt(Widget w, short *value, int max, int neg)
{
    String text;
    long tmpLong;

    XtVaGetValues(w, XtNstring, &text, NULL);
    tmpLong= atol(text);

    if (neg && tmpLong >= -max && tmpLong <= max && NumStringOK(text, 1)) 
        *value= (short)tmpLong;
    else if (!neg && tmpLong >= 1 && tmpLong <= max && NumStringOK(text, 1))
        *value= (short)tmpLong;
    else if (*text == 0) *value= 0;
    else {
        long ipoint= XawTextGetInsertionPoint(w);
        SetWidgetLongText(w, (long)*value, 1);
        XawTextSetInsertionPoint(w, ipoint -1);
        XBell(appDisplay, 0);
    }
}

static void SetValidLong(Widget w, long *value)
{
    String text;
    long tmpLong;

    XtVaGetValues(w, XtNstring, &text, NULL);
    tmpLong= atol(text);
    if (tmpLong >= 0L && strlen(text) < 10 && NumStringOK(text, 1))
        *value= tmpLong;
    else {
        long ipoint= XawTextGetInsertionPoint(w);
        SetWidgetLongText(w, (long)*value, 1);
        XawTextSetInsertionPoint(w, ipoint -1);
        XBell(appDisplay, 0);
    }
}


static void SetValidDouble(Widget w, float *value, float max, short neg)
{
    String text;
    double tmpDbl;
        
    XtVaGetValues(w, XtNstring, &text, NULL);
    tmpDbl= atof(text);
    if (neg && tmpDbl >= -max && tmpDbl <= max && NumStringOK(text, 0)) *value= tmpDbl;
    else if (!neg && tmpDbl >= 0.0 && tmpDbl <= max && NumStringOK(text, 0)) *value= tmpDbl;
    else {
        long ipoint= XawTextGetInsertionPoint(w);
        SetWidgetDoubleText(w, (double)*value);
        XawTextSetInsertionPoint(w, ipoint -1);
        XBell(appDisplay, 0);
    }
}


/* NumStringOK is used by the SetValid... funcs.  It returns true if str is an
   OK string of numbers.  If no_dec_pt is true it won't allow decimal points.
*/

int NumStringOK(char *str, short no_dec_pt)
{
    short i, c;
    for (i= 0; (c= str[i]); i++) {
        if (c >= '0' && c <= '9') continue; /* 0 - 9 is OK */
        if (c == '-' && i == 0) continue;   /* a minus in position 0 is OK */
        if (c != '.') return 0;             /* if not a decimal, it's bad! */
        if (no_dec_pt++) return 0;          /* even a decimal may be bad.  */
    }

    return 1;
}


static void CheckTrainOpts(void)
{
    static int lastrun= 0;

    SetValidInt(seedText, &TrainOpts.userSeed, 32767, 1);
    SetValidInt(bpttText, &TrainOpts.bpttCopies, 100, 0);
    SetValidDouble(rateText, &TrainOpts.learnRate, 1.0, 0);
    SetValidDouble(momentumText, &TrainOpts.momentum, 1.0, 0);
    SetValidDouble(offsetText, &TrainOpts.initialBias, 1000.0, 1);
    SetValidDouble(stopText, &TrainOpts.errCriterion, 100.0, 0);
    SetValidLong(sweepsText, &TrainOpts.sweeps);
    SetValidLong(reportText, &TrainOpts.errorInterval);
    SetValidLong(checkText, &TrainOpts.dumpInterval);
    SetValidLong(updateText, &TrainOpts.updateInterval);

    XtSetSensitive(trainOK, (TrainOpts.learnRate > 0.0 && TrainOpts.sweeps > 0));

    if (runNumber <= 1 && XtSpecificationRelease > 4) 
        XtCallActionProc(lastButton, "stop", 0, 0, 0);  /* stops the repeater */
    if (runNumber > gLastRun) {
        if (XtIsManaged(removeButton)) XtUnmanageChild(removeButton);
        if (runNumber != lastrun && XtSpecificationRelease > 4) 
            XtCallActionProc(nextButton, "stop", 0, 0, 0);
    }

    XtSetSensitive(lastButton, (runNumber > 1));
    XtSetSensitive(nextButton, (runNumber <= gLastRun));


    /* The following code kicks in only when the user has started to
       look at old run settings.  If the user tries to change an old
       setting, the old run is used as the new run, so the run number
       is changed and the title in the window is changed to reflect
       this.  This is necessary because old runs are read-only except
       for their names.  The tmpState structure is used to see if the 
       user has changed anything.  It doesn't look at the last 40
       bytes of the array because that is the runName which is OK to
       change.  */

    if (runNumber <= gLastRun) {

        XtManageChild(removeButton);

        if (runNumber == lastrun) {
            if (memcmp((void *)&tmpState, (void *)&TrainOpts, sizeof(TrainOpts) - 40)) {
                runNumber= gLastRun +1;
                strcpy(TrainOpts.runName, recentOptions.runName);
                SetRunWindowTitle();
            }
        }    

        tmpState= TrainOpts;
        lastrun= runNumber;
    }
}


static void CheckTestOpts(void)
{
    SetValidLong(testSweepsText, &TestOpts.sweeps);
    XtSetSensitive(testOK, (TestOpts.sweeps > 0L || TestOpts.autoSweeps));

    if (TestOpts.calculateError) {
        XtSetSensitive(testLogErrorCBLabel, 1);
        XtSetSensitive(testLogErrorCB, 1);
    }
    else {
        if (TestOpts.writeError) {
            TestOpts.writeError= 0;
            SetCheckBox(testLogErrorCB, 0);
        }
        XtSetSensitive(testLogErrorCBLabel, 0);
        XtSetSensitive(testLogErrorCB, 0);
    }
}


static void CheckSetProj(void)
{
    String text;
        
    XtVaGetValues(setProjText, XtNstring, &text, NULL);
    XtSetSensitive(setProjOK, *text);
}



#define HIDDEN_WIDGETS 32

static void ManageTrainOptsWidgets(void)
{
    Widget children[HIDDEN_WIDGETS];

    children[0]= rmsErrorRB;
    children[1]= c_eErrorRB;
    children[2]= bothErrorRB;
    children[3]= offsetLabel;
    children[4]= offsetText;
    children[5]= logErrorCB;
    children[6]= reportText;
    children[7]= sweeps1;
    children[8]= sweeps2;
    children[9]= sweeps3;
    children[10]= logWeightsCB;
    children[11]= checkText;
    children[12]= loadWeightsCB;
    children[13]= weightsText;
    children[14]= stopLabel;
    children[15]= stopText;
    children[16]= bpttCB;
    children[17]= bpttText;
    children[18]= copies1;
    children[19]= updateLabel;
    children[20]= updateText;
    children[21]= resetCB;
    children[22]= teachForceCB;

    children[23]= rmsErrorRBLabel;
    children[24]= c_eErrorRBLabel;
    children[25]= bothErrorRBLabel;
    children[26]= logErrorCBLabel;
    children[27]= logWeightsCBLabel;
    children[28]= loadWeightsCBLabel;
    children[29]= bpttCBLabel;
    children[30]= resetCBLabel;
    children[31]= teachForceCBLabel;

    XtVaSetValues(trainMore, XtNlabel, 
                  TestOpts.bigDialog ? " Less... " : " More... ", NULL);
    
    XawFormDoLayout(trainOptsForm, False);
    if (TestOpts.bigDialog) {
        XtVaSetValues(seedWithRB, XtNhorizDistance, 30, NULL);
        XtVaSetValues(seedRandRB, XtNhorizDistance, 30, NULL);
        XtVaSetValues(trainSequRB, XtNhorizDistance, 30, NULL);
        XtVaSetValues(trainRandRB, XtNhorizDistance, 30, NULL);

        XtVaSetValues(trainButtonGap, XtNfromVert, bothErrorRB, NULL);
        XtVaSetValues(seedWithRB, XtNfromVert, offsetLabel, NULL);
        XtVaSetValues(seedWithRBLabel, XtNfromVert, offsetLabel, NULL);
        XtVaSetValues(seedText, XtNfromVert, offsetLabel, NULL);
        XawFormDoLayout(trainOptsForm, True);
        XtVaSetValues(trainOptsShell, XtNallowShellResize, True, NULL);
        XtManageChildren(children, HIDDEN_WIDGETS);
    }
    else {
        XtVaSetValues(seedWithRB, XtNhorizDistance, 60, NULL);
        XtVaSetValues(seedRandRB, XtNhorizDistance, 60, NULL);
        XtVaSetValues(trainSequRB, XtNhorizDistance, 60, NULL);
        XtVaSetValues(trainRandRB, XtNhorizDistance, 60, NULL);

        XtVaSetValues(trainButtonGap, XtNfromVert, trainRandRB, NULL);
        XtVaSetValues(seedWithRB, XtNfromVert, sweepsLabel, NULL);
        XtVaSetValues(seedWithRBLabel, XtNfromVert, sweepsLabel, NULL);
        XtVaSetValues(seedText, XtNfromVert, sweepsLabel, NULL);
        XawFormDoLayout(trainOptsForm, True);
        XtVaSetValues(trainOptsShell, XtNallowShellResize, True, NULL);
        XtUnmanageChildren(children, HIDDEN_WIDGETS);
    }

    XtVaSetValues(trainOptsShell, XtNallowShellResize, False, NULL);
}



static void ToggleBigTrainOpts(Widget w, XtPointer client_data, XtPointer call_data)
{
    TestOpts.bigDialog= !TestOpts.bigDialog;
    
    ManageTrainOptsWidgets();
    SaveTestingOptions();  /* remember big dialog for next time */
}



static void UpdateStatusWindowButtons(void)
{
    XtSetSensitive(weightsButton, (gRunning && learning));
    XtSetSensitive(abortButton, gRunning);
}


#define ActionString verify?"Verifying":probe?"Probing":"Training"  

static void UpdateStatusText(char *state)
{
    sprintf(buf, "\n\n  %s %s", ActionString, state);
    XReportCondition(buf, 0);
}
        

static void UpdateStatusSweeps(void)
{
    *buf= 0; 
    if (sweep != 0L) sprintf(buf, "%ld", sweep);
    XtVaSetValues(sweepCount, XtNlabel, (String)buf, NULL);
}


static void Quit(Widget w, XtPointer client_data, XtPointer call_data)
{ 
    exit(0); 
}


    /* Prepares for run and sets global variables telling event
       loop to run cycle routine. */

static void StartSystem(Widget w, XtPointer client_data, XtPointer call_data)
{
    tsweeps = 0;
    sweep = 0;

    probe= (w == mTestPane[2]);
    verify= (w == mTestPane[1]); 
    resume= (w == mTrainPane[2]);
    learning= (w == mTrainPane[1] || resume);

    aResetMenuBar(0, 0, 0, 0);
    XReportCondition("\n\nInitializing, please wait...", 0);
    ProcessPendingXEvents();  /* make menu reset and init msg appear now. */

    if (set_up_simulation()) return;

    if ((probe || verify) && OutputWindowHeader()) return;
    if (learning) {
        X_save_wts(0, 0, 0);
        if (!fixedseed) TrainOpts.userSeed= seed;
        SaveTrainingOptions(0);
    }

    gRunning= 1;     /* Set global flag to start system */

    UpdateStatusWindowButtons();
    UpdateStatusText("the network...");
    UpdateStatusSweeps();
}


    /* Initialize entire interface.  This function is huge.   */
    /* Create all widgets, set geometry, and attach callbacks */
    /* and actions.  Then finally Realize the whole mess.     */

void InitXInterface(int argc, char **argv)
{
    int i;
    Dimension width, maxwidth;
    XGCValues gcVals;
    XtTranslations 
        overrideTrans, 
        menuCommTrans, 
        editTextTrans,
        testToggleTrans,
        trainToggleTrans;

    static XtActionsRec actions[]= {
        {"aMaintainMenus", aMaintainMenus},
        {"aResetMenuBar", aResetMenuBar},
        {"aTrainOptsDlgToggles", aTrainOptsDlgToggles},
        {"aTestOptsDlgToggles", aTestOptsDlgToggles},
        {"aSpecialInsertChar", aSpecialInsertChar},
        {"aSpecialDeletePrev", aSpecialDeletePrev},
        {"aSetFocus", aSetFocus},
        {"aPressOK", aPressOK},
        {"aUnSelect", aUnSelect}
    };


/* These resources may be changed w/o recompiling by putting one 
   or more in an app-defaults file named XTlearn.  The file should
   be put in either the users home directory or in the system's 
   app-defaults directory, usually /usr/lib/X11/app-defaults.
   Your system administrator should know where to put it. */

    static char *fallback_resources[]= {
        "XTlearn*mFileBar.label:  File",
        "XTlearn*mTrainBar.label: Train",
        "XTlearn*mTestBar.label:  Test",
        "XTlearn*mDispBar.label:  Displays",
        "XTlearn*mFilePane0.label: About OSXtlearn...",
        "XTlearn*mFilePane1.label: Set Project",
        "XTlearn*mFilePane2.label: Quit",
        "XTlearn*mTrainPane0.label: Training Options...",
        "XTlearn*mTrainPane1.label: Train the network",
        "XTlearn*mTrainPane2.label: Resume training",
        "XTlearn*mTrainPane3.label: Abort",
        "XTlearn*mTestPane0.label: Testing Options...",
        "XTlearn*mTestPane1.label: Verify network has learned",
        "XTlearn*mTestPane2.label: Probe selected nodes",
        "XTlearn*mTestPane3.label: Abort",
        "XTlearn*mDispPane0.label: Error display",
        "XTlearn*mDispPane1.label: Node Activations",
        "XTlearn*mDispPane2.label: Connection Weights",
        "XTlearn*mDispPane3.label: Network Architecture",
        "XTlearn*mDispPane4.label: Probe/Verify Output",
        "XTlearn*abortBtn.label:  Abort",
        "XTlearn*dumpWts.label:   Dump Weights",
        "XTlearn*foreground: black",
        "XTlearn*background: white",
        "XTlearn*font: *helvetica-bold-r-*-14-*",
        "XTlearn*Command.highlightThickness: 0",
        "XTlearn*Repeater.highlightThickness: 0",
        "XTlearn*Command.background: gray95",
        "XTlearn*Repeater.background: gray95",
        "XTlearn*mainForm*Command*background: white",
        "XTlearn*mainForm*statusBar*Command*background: gray95",
        "XTlearn*Label*borderWidth: 0",
        "XTlearn*trainOptsForm*runNameText*Background: white",
        "XTlearn*statusBar*font:  *Helvetica-bold-r-*-12-*",
        "XTlearn*outputText*font: *fixed-bold-r*13-*-c-*",
        "XTlearn*infoArea*font: *fixed-bold-r*13-*-c-*",
        (char *)NULL
    };


    appWidget= XtAppInitialize(&appContext, "XTlearn", 
        (XrmOptionDescRec*)NULL, 0, (ARGCTYPE *)&argc, argv, 
        fallback_resources, 0, 0);

    appDisplay= XtDisplay(appWidget);
    appWindow= RootWindowOfScreen(XtScreen(appWidget));
    appDepth= DefaultDepthOfScreen(XtScreen(appWidget));

    XtAppAddActions(appContext, actions, XtNumber(actions));

    overrideTrans= XtParseTranslationTable("<BtnUp>: aResetMenuBar()");
    menuCommTrans= XtParseTranslationTable(
        "<EnterWindow>:  set()       \n\
         <LeaveWindow>:  unset()     \n\
         <BtnUp>:        notify() unset()");

    editTextTrans= XtParseTranslationTable(
        "Ctrl<Key>A:     aUnSelect() beginning-of-line() \n\
         Ctrl<Key>B:     aUnSelect() backward-character() \n\
         Ctrl<Key>D:     aUnSelect() delete-next-character() \n\
         Ctrl<Key>E:     aUnSelect() end-of-line() \n\
         Ctrl<Key>F:     aUnSelect() forward-character() \n\
         Ctrl<Key>H:     aSpecialDeletePrev() \n\
         Ctrl<Key>K:     kill-to-end-of-line() \n\
         Ctrl<Key>L:     redraw-display() \n\
         Ctrl<Key>T:     transpose-characters() \n\
         Ctrl<Key>W:     kill-selection() \n\
         Ctrl<Key>Y:     insert-selection(CUT_BUFFER1) \n\
         <Key>Right:     aUnSelect() forward-character()  \n\
         <Key>Left:      aUnSelect() backward-character() \n\
         <Key>Delete:    aSpecialDeletePrev() \n\
         <Key>BackSpace: aSpecialDeletePrev() \n\
         <Key>Return:    aPressOK() \n\
         <Key>:          aSpecialInsertChar()   \n\
         <Btn1Down>:     aSetFocus() select-start() \n\
         <Btn1Motion>:   extend-adjust() \n\
         <Btn1Up>:       extend-end(PRIMARY, CUT_BUFFER0) \n\
         <Btn2Down>:     insert-selection(PRIMARY, CUT_BUFFER0) \n\
         <Btn3Down>:     extend-start() \n\
         <Btn3Motion>:   extend-adjust() \n\
         <Btn3Up>:       extend-end(PRIMARY, CUT_BUFFER0) \n\
         <FocusIn>:      display-caret(on) \n\
         <FocusOut>:     aUnSelect() display-caret(off) \n\
         <Enter>:        no-op() \n\
         <Leave>:        no-op()");

    testToggleTrans= XtParseTranslationTable(
        "<Leave>:            reset()    \n\
         <BtnDown>,<BtnUp>: aTestOptsDlgToggles()");

    trainToggleTrans= XtParseTranslationTable(
        "<Leave>:            reset()    \n\
         <BtnDown>,<BtnUp>: aTrainOptsDlgToggles()");

    /* Do all the icon init stuff */

        /* Create all of our bitmaps from the data files. */
    checkedCB= XCreateBitmapFromData(appDisplay, appWindow, 
        checkedCB_bits, checkedCB_width, checkedCB_height);

    uncheckedCB= XCreateBitmapFromData(appDisplay, appWindow, 
        uncheckedCB_bits, uncheckedCB_width, uncheckedCB_height);

    checkedRB= XCreateBitmapFromData(appDisplay, appWindow, 
        checkedRB_bits, checkedRB_width, checkedRB_height);

    uncheckedRB= XCreateBitmapFromData(appDisplay, appWindow, 
        uncheckedRB_bits, uncheckedRB_width, uncheckedRB_height);

    vertIcon= XCreateBitmapFromData(appDisplay, appWindow, 
        vertIcon_bits, vertIcon_width, vertIcon_height);

    horizIcon= XCreateBitmapFromData(appDisplay, appWindow,
        horizIcon_bits, horizIcon_width, horizIcon_height);

    grayStipple= XCreateBitmapFromData(appDisplay, appWindow,
        grayStipple_bits, grayStipple_width, grayStipple_height);

    /* This is the main form widget; home of the main menu bar */

    mainForm= XtVaCreateManagedWidget("mainForm", 
        formWidgetClass, appWidget, NULL);


    /* Create the Widgets that make up the main menu bar */

    mFileBar= XtVaCreateManagedWidget("mFileBar",
        commandWidgetClass, mainForm, LOCK_GEOMETRY, 
        XtNborderWidth, 0,
        XtNtranslations, XtParseTranslationTable(
        "<BtnDown>:  aMaintainMenus() set() XtMenuPopup(mFileShell)"),
        NULL);

    mTrainBar= XtVaCreateManagedWidget("mTrainBar", 
        commandWidgetClass, mainForm, LOCK_GEOMETRY, 
        XtNborderWidth, 0,
        XtNfromHoriz, mFileBar,
        XtNtranslations, XtParseTranslationTable(
        "<BtnDown>:  aMaintainMenus() set() XtMenuPopup(mTrainShell)"),
        NULL);

    mTestBar= XtVaCreateManagedWidget("mTestBar", 
        commandWidgetClass, mainForm, LOCK_GEOMETRY, 
        XtNborderWidth, 0,
        XtNfromHoriz, mTrainBar,
        XtNtranslations, XtParseTranslationTable(
        "<BtnDown>:  aMaintainMenus() set() XtMenuPopup(mTestShell)"),
        NULL);

    mDispBar= XtVaCreateManagedWidget("mDispBar", 
        commandWidgetClass, mainForm, LOCK_GEOMETRY, 
        XtNborderWidth, 0,
        XtNfromHoriz, mTestBar,
        XtNtranslations, XtParseTranslationTable(
        "<BtnDown>:  aMaintainMenus() set() XtMenuPopup(mDispShell)"),
        NULL);


    /* Create the popup shell and box widgets for the pull down menus */

    mFileShell= XtVaCreatePopupShell("mFileShell", 
        overrideShellWidgetClass, mFileBar, 
        XtNtranslations, overrideTrans,
        NULL);

    mTrainShell= XtVaCreatePopupShell("mTrainShell", 
        overrideShellWidgetClass, mTrainBar,
        XtNtranslations, overrideTrans,
        NULL);

    mTestShell= XtVaCreatePopupShell("mTestShell", 
        overrideShellWidgetClass, mTestBar,
        XtNtranslations, overrideTrans,
        NULL);

    mDispShell= XtVaCreatePopupShell("mDispShell", 
        overrideShellWidgetClass, mDispBar,
        XtNtranslations, overrideTrans,
        NULL);

    mFileBox= XtVaCreateManagedWidget("mFileBox",
        boxWidgetClass, mFileShell,
        XtNvSpace, 1, XtNhSpace, 1,
        NULL);

    mTrainBox= XtVaCreateManagedWidget("mTrainBox",
        boxWidgetClass, mTrainShell,
        XtNvSpace, 1, XtNhSpace, 1,
        NULL);

    mTestBox= XtVaCreateManagedWidget("mTrainBox",
        boxWidgetClass, mTestShell,
        XtNvSpace, 1, XtNhSpace, 1,
        NULL);

    mDispBox= XtVaCreateManagedWidget("mDispBox",
        boxWidgetClass, mDispShell, 
        XtNvSpace, 1, XtNhSpace, 1,
        NULL);


    /* Create the panes for the menus and make them all the same width */

    for (i= 0, maxwidth= 0; i < 3; i++) {
        sprintf(buf, "mFilePane%d", i);
        mFilePane[i]= XtVaCreateManagedWidget((String)buf, 
            commandWidgetClass, mFileBox, 
            XtNborderWidth, 0,
            XtNjustify, XtJustifyLeft, 
            XtNtranslations, menuCommTrans,
            NULL);

        XtVaGetValues(mFilePane[i], XtNwidth, &width, NULL);
        if (width > maxwidth) maxwidth= width;
    }

    for (i= 0; i < 3; i++) 
        XtVaSetValues(mFilePane[i], XtNwidth, maxwidth, NULL);


    for (i= 0, maxwidth= 0; i < 4; i++) {
        sprintf(buf, "mTrainPane%d", i);
        mTrainPane[i]= XtVaCreateManagedWidget((String)buf, 
            commandWidgetClass, mTrainBox,
            XtNborderWidth, 0,
            XtNjustify, XtJustifyLeft, 
            XtNtranslations, menuCommTrans,
            NULL);

        XtVaGetValues(mTrainPane[i], XtNwidth, &width, NULL);
        if (width > maxwidth) maxwidth= width;
    }

    for (i= 0; i < 4; i++) 
        XtVaSetValues(mTrainPane[i], XtNwidth, maxwidth, NULL);


    for (i= 0, maxwidth= 0; i < 4; i++) {
        sprintf(buf, "mTestPane%d", i);
        mTestPane[i]= XtVaCreateManagedWidget((String)buf, 
            commandWidgetClass, mTestBox, 
            XtNborderWidth, 0,
            XtNjustify, XtJustifyLeft, 
            XtNtranslations, menuCommTrans,
            NULL);

        XtVaGetValues(mTestPane[i], XtNwidth, &width, NULL);
        if (width > maxwidth) maxwidth= width;
    }

    for (i= 0; i < 4; i++) 
        XtVaSetValues(mTestPane[i], XtNwidth, maxwidth, NULL);


    for (i= 0, maxwidth= 0; i < 5; i++) {
        sprintf(buf, "mDispPane%d", i);
        mDispPane[i]= XtVaCreateManagedWidget((String)buf, 
            commandWidgetClass, mDispBox, 
            XtNborderWidth, 0,
            XtNjustify, XtJustifyLeft, 
            XtNtranslations, menuCommTrans,
            NULL);

        XtVaGetValues(mDispPane[i], XtNwidth, &width, NULL);
        if (width > maxwidth) maxwidth= width;
    }

    for (i= 0; i < 5; i++) 
        XtVaSetValues(mDispPane[i], XtNwidth, maxwidth, NULL);


    /* Create the status bar and its child widgets */

    statusBar= XtVaCreateManagedWidget("statusBar",
        formWidgetClass, mainForm,
        XtNtop, XtChainTop,
        XtNbottom, XtChainTop,
        XtNleft, XtChainLeft,
        XtNfromVert, mTestBar,
        NULL);

    projLabel= XtVaCreateManagedWidget("projLabel",
        labelWidgetClass, statusBar, LOCK_GEOMETRY,
        XtNborderWidth, 0,
        XtNjustify, XtJustifyLeft,
        XtNlabel, "Project:",
        NULL);

    XtVaGetValues(projLabel, XtNwidth, &maxwidth, NULL);

    projName= XtVaCreateManagedWidget("projName",  
        labelWidgetClass, statusBar, LOCK_GEOMETRY,
        XtNborderWidth, 0,
        XtNjustify, XtJustifyLeft,
        XtNlabel, "<not specified>         ",
        XtNfromHoriz, projLabel,
        XtNhorizDistance, 2,
        NULL);

    XtVaGetValues(projName, XtNwidth, &width, NULL);          /* get this width */
    maxwidth += (width +width);

    sweepCountLabel= XtVaCreateManagedWidget("sweepCountLabel",  
        labelWidgetClass, statusBar, LOCK_GEOMETRY,
        XtNborderWidth, 0,
        XtNjustify, XtJustifyLeft,
        XtNlabel, "Sweep:",
        XtNfromHoriz, projName,
        XtNhorizDistance, 2,
        NULL);

    sweepCount= XtVaCreateManagedWidget("sweepCount",  
        labelWidgetClass, statusBar,  LOCK_GEOMETRY,  
        XtNborderWidth, 0,
        XtNjustify, XtJustifyLeft,
        XtNlabel, " ",
        XtNwidth, width,   /* same width as projName */
        XtNfromHoriz, sweepCountLabel,
        NULL);

    abortButton= XtVaCreateManagedWidget("abortButton", 
        commandWidgetClass, statusBar, LOCK_GEOMETRY, 
        XtNsensitive, 0,
        XtNfromHoriz, sweepCount,
        XtNlabel, "Abort",
        NULL);

    weightsButton= XtVaCreateManagedWidget("weightsButton", 
        commandWidgetClass, statusBar, LOCK_GEOMETRY, 
        XtNsensitive, 0,
        XtNfromHoriz, abortButton,
        XtNlabel, "Dump Weights",
        NULL);

    XtVaGetValues(sweepCountLabel, XtNwidth, &width, NULL);  /* Make the buttons */
    maxwidth += width;
    XtVaGetValues(weightsButton, XtNwidth, &width, NULL);  /* Make the buttons */
    XtVaSetValues(abortButton, XtNwidth, width, NULL);     /*  the same width. */
    maxwidth += (width +width);


    /* Creat the info area widget */

    infoArea= XtVaCreateManagedWidget("infoArea",
        asciiTextWidgetClass, mainForm, LOCK_GEOMETRY,
        XtNeditType, XawtextRead,
        XtNtop, XtChainTop,
        XtNbottom, XtChainBottom,
        XtNleft, XtChainLeft,
        XtNfromVert, statusBar,
        XtNwidth, maxwidth +28,
        XtNheight, 85,
        XtNborderWidth, 0,
        XtNsensitive, FALSE,
        XtNcursor, NULL,
        XtNdisplayCaret, 0,
        XtNstring, "\n\nWelcome to OSXtlearn.",
        NULL);

    setProjShell= XtVaCreatePopupShell("xtlearn Set Project Name", 
        transientShellWidgetClass, appWidget, NULL);

    setProjForm= XtVaCreateManagedWidget("setProjForm",
        formWidgetClass, setProjShell, NULL);

    setProjLabel= XtVaCreateManagedWidget("setProjLabel",
        labelWidgetClass, setProjForm, LOCK_GEOMETRY, 
        XtNhorizDistance, 15,
        XtNvertDistance, 15,
        XtNlabel, "Project name:",
        NULL);

    setProjText= XtVaCreateManagedWidget("setProjText",
        asciiTextWidgetClass, setProjForm, LOCK_GEOMETRY, 
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNhorizDistance, 20,
        XtNfromVert, setProjLabel,
        XtNwidth, 200,
        NULL);

    setProjCancel= XtVaCreateManagedWidget("setProjCancel",  
        commandWidgetClass, setProjForm,    
        LOCK_GEOMETRY,
        XtNlabel, " Cancel ",
        XtNfromVert, setProjText,
        XtNvertDistance, 10,
        XtNhorizDistance, 130,
        NULL);

    setProjOK= XtVaCreateManagedWidget("setProjOK",          
        commandWidgetClass, setProjForm, LOCK_GEOMETRY,
        XtNborderWidth, 4,
        XtNlabel, "    OK    ",
        XtNfromVert, setProjText,
        XtNfromHoriz, setProjCancel,
        XtNvertDistance, 8,
        XtNhorizDistance, 15,
        NULL);

    XtAddCallback(mFilePane[1], XtNcallback, DialogPopup, (XtPointer)setProjShell);
    XtAddCallback(setProjShell, XtNpopupCallback, InitSetProjDialog, NULL);
    XtAddCallback(setProjCancel, XtNcallback, DialogPopdown, (XtPointer)setProjShell);
    XtAddCallback(setProjOK, XtNcallback, OKSetProjDialog, (XtPointer)setProjShell);


    /* Create the Training Opts dialog and all its child widgets */

    trainOptsShell= XtVaCreatePopupShell("xtlearn Training Options", 
        transientShellWidgetClass, appWidget, NULL);

    trainOptsForm= XtVaCreateManagedWidget("trainOptsForm",
        formWidgetClass, trainOptsShell, NULL);
    
    runNameText= XtVaCreateManagedWidget("runNameText",
        commandWidgetClass, trainOptsForm,
        XtNtop, XtChainTop,
        XtNbottom, XtChainTop,
        XtNleft, XtChainLeft,
        XtNright, XtChainRight,
        XtNborderWidth, 0,
        XtNvertDistance, 0,
        XtNwidth, 522,
        NULL);

    XtVaCreateManagedWidget("line",
        labelWidgetClass, trainOptsForm,
        XtNtop, XtChainTop,
        XtNbottom, XtChainTop,
        XtNleft, XtChainLeft,
        XtNright, XtChainRight,
        XtNfromVert, runNameText,
        XtNwidth, 520,
        XtNheight, 1,
        XtNvertDistance, 0,
        XtNborderWidth, 1,
        XtNlabel, " ",
        NULL);

    sweepsLabel= XtVaCreateManagedWidget("sweepsLabel",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNjustify, XtJustifyLeft,
        XtNfromVert, runNameText,
        XtNvertDistance, 15,
        XtNhorizDistance, 30,
        XtNlabel, "Training Sweeps:",
        NULL);

    sweepsText= XtVaCreateManagedWidget("sweepsText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, runNameText,
        XtNfromHoriz, sweepsLabel,
        XtNvertDistance, 15,
        XtNwidth, 75,
        XtNdisplayCaret, 0,
        NULL);

    rateLabel= XtVaCreateManagedWidget("rateLabel",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNjustify, XtJustifyLeft,
        XtNfromVert, runNameText,
        XtNfromHoriz, sweepsText,
        XtNvertDistance, 15,
        XtNhorizDistance, 30,
        XtNlabel, "Learning Rate:",
        NULL);

    rateText= XtVaCreateManagedWidget("rateText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, runNameText,
        XtNfromHoriz, rateLabel,
        XtNvertDistance, 15,
        XtNwidth, 50,
        XtNdisplayCaret, 0,
        NULL);

    offsetLabel= XtVaCreateManagedWidget("offsetLabel",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNjustify, XtJustifyLeft,
        XtNfromVert, sweepsLabel,
        XtNhorizDistance, 47,
        XtNlabel, "Init bias offset:",
        NULL);

    offsetText= XtVaCreateManagedWidget("offsetText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, sweepsLabel,
        XtNfromHoriz, offsetLabel,
        XtNdisplayCaret, 0,
        XtNwidth, 50,
        NULL);

    momentumLabel= XtVaCreateManagedWidget("momentumLabel",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNjustify, XtJustifyLeft,
        XtNfromVert, sweepsLabel,
        XtNfromHoriz, sweepsText,
        XtNhorizDistance, 50,
        XtNlabel, "Momentum:",
        NULL);

    momentumText= XtVaCreateManagedWidget("momentumText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, sweepsLabel,
        XtNfromHoriz, momentumLabel,
        XtNdisplayCaret, 0,
        XtNwidth, 50,
        NULL);

    seedWithRB= XtVaCreateManagedWidget("seedWithRB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, offsetLabel,
        XtNvertDistance, 15,
        XtNhorizDistance, 30,
        NULL);

    seedWithRBLabel= XtVaCreateManagedWidget("seedWithRBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, offsetLabel,
        XtNfromHoriz, seedWithRB,
        XtNvertDistance, 15,
        XtNhorizDistance, 0,
        XtNlabel, "Seed with:",
        NULL);

    seedText= XtVaCreateManagedWidget("seedText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, offsetLabel,
        XtNfromHoriz, seedWithRBLabel,
        XtNvertDistance, 15,
        XtNwidth, 60,
        XtNdisplayCaret, 0,
        NULL);

    seedRandRB= XtVaCreateManagedWidget("seedRandRB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, seedWithRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 30,
        NULL);

    seedRandRBLabel= XtVaCreateManagedWidget("seedRandRBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, seedWithRB,
        XtNfromHoriz, seedRandRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 0,
        XtNlabel, "Seed randomly",
        NULL);

    trainSequRB= XtVaCreateManagedWidget("trainSequRB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, seedRandRB,
        XtNvertDistance, 12,
        XtNhorizDistance, 30,
        NULL);

    trainSequRBLabel= XtVaCreateManagedWidget("trainSequRBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, seedRandRB,
        XtNfromHoriz, trainSequRB,
        XtNvertDistance, 12,
        XtNhorizDistance, 0,
        XtNlabel, "Train sequentially",
        NULL);

    trainRandRB= XtVaCreateManagedWidget("trainRandRB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNjustify, XtJustifyLeft,
        XtNborderWidth, 0,
        XtNfromVert, trainSequRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 30,
        NULL);

    trainRandRBLabel= XtVaCreateManagedWidget("trainRandRBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNjustify, XtJustifyLeft,
        XtNborderWidth, 0,
        XtNfromVert, trainSequRB,
        XtNfromHoriz, trainRandRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 0,
        XtNlabel, "Train randomly",
        NULL);

    rmsErrorRB= XtVaCreateManagedWidget("rmsErrorRB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, trainRandRB,
        XtNvertDistance, 12,
        XtNhorizDistance, 30,
        NULL);

    rmsErrorRBLabel= XtVaCreateManagedWidget("rmsErrorRBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, trainRandRB,
        XtNfromHoriz, rmsErrorRB,
        XtNvertDistance, 12,
        XtNhorizDistance, 0,
        XtNlabel, "Use & log RMS error",
        NULL);

    c_eErrorRB= XtVaCreateManagedWidget("c_eErrorRB",
        toggleWidgetClass, trainOptsForm,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        LOCK_GEOMETRY, 
        XtNfromVert, rmsErrorRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 30,
        NULL);

    c_eErrorRBLabel= XtVaCreateManagedWidget("c_eErrorRBLabel",
        toggleWidgetClass, trainOptsForm,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        LOCK_GEOMETRY, 
        XtNfromVert, rmsErrorRB,
        XtNfromHoriz, c_eErrorRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 0,
        XtNlabel, "Use & log X-entropy",
        NULL);

    bothErrorRB= XtVaCreateManagedWidget("bothErrorRB",
        toggleWidgetClass, trainOptsForm,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        LOCK_GEOMETRY, 
        XtNfromVert, c_eErrorRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 30,
        NULL);

    bothErrorRBLabel= XtVaCreateManagedWidget("bothErrorRBLabel",
        toggleWidgetClass, trainOptsForm,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        LOCK_GEOMETRY, 
        XtNfromVert, c_eErrorRB,
        XtNfromHoriz, bothErrorRB,
        XtNvertDistance, 0,
        XtNhorizDistance, 0,
        XtNlabel, "Use X-ent; log RMS",
        NULL);

    trainButtonGap= XtVaCreateManagedWidget("trainButtonGap",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNlabel, " ", 
        XtNfromVert, bothErrorRB,
        NULL);

    trainMore= XtVaCreateManagedWidget("trainMore",
        commandWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, trainButtonGap,
        XtNhorizDistance, 30,
        NULL);

    removeButton= XtVaCreateManagedWidget("removeButton",
        commandWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNlabel, "Remove run",
        XtNfromVert, trainButtonGap,
        XtNfromHoriz, trainMore,
        XtNhorizDistance, 10,
        NULL);

    lastButton= XtVaCreateManagedWidget("lastButton",
        repeaterWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNlabel, "Prev",
        XtNfromVert, trainButtonGap,
        XtNfromHoriz, trainMore,
        XtNhorizDistance, 110,
        XtNwidth, 40,
        NULL);

    nextButton= XtVaCreateManagedWidget("nextButton",
        repeaterWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNlabel, "Next",
        XtNfromVert, trainButtonGap,
        XtNfromHoriz, lastButton,
        XtNwidth, 40,
        NULL);

    trainCancel= XtVaCreateManagedWidget("trainCancel",  
        commandWidgetClass, trainOptsForm,    
        LOCK_GEOMETRY,
        XtNlabel, " Cancel ",
        XtNfromHoriz, nextButton,
        XtNfromVert, trainButtonGap,
        XtNhorizDistance, 60,
        NULL);

    trainOK= XtVaCreateManagedWidget("trainOK",          
        commandWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNborderWidth, 4,
        XtNlabel, "    OK    ",
        XtNfromVert, trainButtonGap,
        XtNfromHoriz, trainCancel,
        XtNvertDistance, 0,
        XtNhorizDistance, 15,
        NULL);

    logErrorCB= XtVaCreateManagedWidget("logErrorCB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, momentumText,
        XtNfromHoriz, seedText,
        XtNvertDistance, 15,
        XtNhorizDistance, 20,
        NULL);

    logErrorCBLabel= XtVaCreateManagedWidget("logErrorCBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, momentumText,
        XtNfromHoriz, logErrorCB,
        XtNvertDistance, 15,
        XtNhorizDistance, 0,
        XtNlabel, "Log error every",
        NULL);

    reportText= XtVaCreateManagedWidget("reportText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, momentumText,
        XtNfromHoriz, logErrorCBLabel,
        XtNwidth, 60,
        XtNvertDistance, 15,
        XtNdisplayCaret, 0,
        NULL);

    sweeps1= XtVaCreateManagedWidget("sweeps",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, momentumText,
        XtNfromHoriz, reportText,
        XtNvertDistance, 15,
        NULL);

    logWeightsCB= XtVaCreateManagedWidget("logWeightsCB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, logErrorCB,
        XtNfromHoriz, seedText,
        XtNhorizDistance, 20,
        NULL);

    logWeightsCBLabel= XtVaCreateManagedWidget("logWeightsCBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, logErrorCB,
        XtNfromHoriz, logWeightsCB,
        XtNhorizDistance, 0,
        XtNlabel, "Dump weights every",
        NULL);

    checkText= XtVaCreateManagedWidget("checkText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, logErrorCB,
        XtNfromHoriz, logWeightsCBLabel,
        XtNwidth, 60,
        XtNdisplayCaret, 0,
        NULL);

    sweeps2= XtVaCreateManagedWidget("sweeps",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, logErrorCB,
        XtNfromHoriz, checkText,
        NULL);

    loadWeightsCB= XtVaCreateManagedWidget("loadWeightsCB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, logWeightsCB,
        XtNfromHoriz, seedText,
        XtNhorizDistance, 20,
        NULL);

    loadWeightsCBLabel= XtVaCreateManagedWidget("loadWeightsCBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, logWeightsCB,
        XtNfromHoriz, loadWeightsCB,
        XtNhorizDistance, 0,
        XtNlabel, "Load weights file:",
        NULL);

    weightsText= XtVaCreateManagedWidget("weightsText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, logWeightsCB,
        XtNfromHoriz, loadWeightsCBLabel,
        XtNwidth, 150,
        XtNdisplayCaret, 0,
        NULL);

    stopLabel= XtVaCreateManagedWidget("stopLabel",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, loadWeightsCB,
        XtNfromHoriz, seedText,
        XtNhorizDistance, 20,
        XtNlabel, "Halt if RMS error falls below",
        NULL);

    stopText= XtVaCreateManagedWidget("stopText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, loadWeightsCB,
        XtNfromHoriz, stopLabel,
        XtNwidth, 60,
        XtNdisplayCaret, 0,
        NULL);

    bpttCB= XtVaCreateManagedWidget("bpttCB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, stopLabel,
        XtNfromHoriz, seedText,
        XtNhorizDistance, 20,
        NULL);

    bpttCBLabel= XtVaCreateManagedWidget("bpttCBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, stopLabel,
        XtNfromHoriz, bpttCB,
        XtNhorizDistance, 0,
        XtNlabel, "Back prop thru time w/",
        NULL);

    bpttText= XtVaCreateManagedWidget("bpttText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, stopLabel,
        XtNfromHoriz, bpttCBLabel,
        XtNwidth, 35,
        XtNdisplayCaret, 0,
        NULL);

    copies1= XtVaCreateManagedWidget("copies",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, stopLabel,
        XtNfromHoriz, bpttText,
        NULL);

    updateLabel= XtVaCreateManagedWidget("updateLabel",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, bpttCB,
        XtNfromHoriz, seedText,
        XtNhorizDistance, 20,
        XtNlabel, "Update weights every",
        NULL);

    updateText= XtVaCreateManagedWidget("updateText",
        asciiTextWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, bpttCB,
        XtNfromHoriz, updateLabel,
        XtNwidth, 50,
        XtNdisplayCaret, 0,
        NULL);

    sweeps3= XtVaCreateManagedWidget("sweeps",
        labelWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNfromVert, bpttCB,
        XtNfromHoriz, updateText,
        NULL);

    teachForceCB= XtVaCreateManagedWidget("teachForceCB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, updateLabel,
        XtNfromHoriz, seedText,
        XtNhorizDistance, 20,
        NULL);

    teachForceCBLabel= XtVaCreateManagedWidget("teachForceCBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, updateLabel,
        XtNfromHoriz, teachForceCB,
        XtNhorizDistance, 0,
        XtNlabel, "Teacher forcing",
        NULL);

    resetCB= XtVaCreateManagedWidget("resetCB",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, updateLabel,
        XtNfromHoriz, teachForceCBLabel,
        XtNhorizDistance, 20,
        NULL);

    resetCBLabel= XtVaCreateManagedWidget("resetCBLabel",
        toggleWidgetClass, trainOptsForm, LOCK_GEOMETRY,
        XtNtranslations, trainToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, updateLabel,
        XtNfromHoriz, resetCB,
        XtNhorizDistance, 0,
        XtNlabel, "Use reset file",
        NULL);

    XtAddCallback(trainMore, XtNcallback, ToggleBigTrainOpts, NULL);
    XtAddCallback(mTrainPane[0], XtNcallback, DialogPopup, (XtPointer)trainOptsShell);
    XtAddCallback(mFilePane[0], XtNcallback, AboutBox, NULL);
    XtAddCallback(mFilePane[2], XtNcallback, Quit, NULL);

    XtAddCallback(trainOptsShell, XtNpopupCallback, InitTrainingDialog, NULL);

    XtAddCallback(mFileShell, XtNpopupCallback, PlaceMenu, (XtPointer)mFileBar);
    XtAddCallback(mTrainShell, XtNpopupCallback, PlaceMenu, (XtPointer)mTrainBar);
    XtAddCallback(mTestShell, XtNpopupCallback, PlaceMenu, (XtPointer)mTestBar);
    XtAddCallback(mDispShell, XtNpopupCallback, PlaceMenu, (XtPointer)mDispBar);

    XtAddCallback(lastButton, XtNcallback, HistoryScroll, NULL);
    XtAddCallback(nextButton, XtNcallback, HistoryScroll, NULL);
    XtAddCallback(removeButton, XtNcallback, RemoveTrainingOptions, NULL);

    XtAddCallback(trainCancel, XtNcallback, CancelTrainingDialog, (XtPointer)trainOptsShell);
    XtAddCallback(trainOK, XtNcallback, OKTrainingDialog, (XtPointer)trainOptsShell);

    XtUnmanageChild(removeButton);

    testOptsShell= XtVaCreatePopupShell("xtlearn Testing Options", 
        transientShellWidgetClass, appWidget, NULL);

    testOptsForm= XtVaCreateManagedWidget("testOptsForm",
        formWidgetClass, testOptsShell, NULL);
    
    testOptsTitle= XtVaCreateManagedWidget("testOptsTitle",
        labelWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNlabel, "Testing Options",
        XtNwidth, 400,
        NULL);

    testWeightsLabel= XtVaCreateManagedWidget("testWeightsLabel",
        labelWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNfromVert, testOptsTitle,
        XtNhorizDistance, 20,
        XtNvertDistance, 10,
        XtNlabel, "Weights file:",
        NULL);

    mostRecentWtsRB= XtVaCreateManagedWidget("mostRecentWtsRB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, testOptsTitle,
        XtNfromHoriz, testWeightsLabel,
        XtNvertDistance, 10,
        NULL);

    mostRecentWtsRBLabel= XtVaCreateManagedWidget("mostRecentWtsRBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, testOptsTitle,
        XtNfromHoriz, mostRecentWtsRB,
        XtNvertDistance, 10,
        XtNhorizDistance, 0,
        XtNlabel, "Most recent",
        NULL);

    mostRecentLabel= XtVaCreateManagedWidget("mostRecentLabel",
        labelWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNfromVert, testOptsTitle,
        XtNfromHoriz, mostRecentWtsRBLabel,
        XtNvertDistance, 10,
        XtNlabel, "<< not available >>",
        NULL);

    alwaysUseWtsRB= XtVaCreateManagedWidget("alwaysUseWtsRB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, testWeightsLabel,
        XtNfromHoriz, testWeightsLabel,
        NULL);

    alwaysUseWtsRBLabel= XtVaCreateManagedWidget("alwaysUseWtsRBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, testWeightsLabel,
        XtNfromHoriz, alwaysUseWtsRB,
        XtNhorizDistance, 0,
        XtNlabel, "Always use:",
        NULL);

    alwaysUseText= XtVaCreateManagedWidget("alwaysUseText",
        asciiTextWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, testWeightsLabel,
        XtNfromHoriz, alwaysUseWtsRBLabel,
        XtNwidth, 150,
        XtNdisplayCaret, 0,
        NULL);

    testDataLabel= XtVaCreateManagedWidget("testDataLabel",
        labelWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNfromVert, alwaysUseWtsRB,
        XtNhorizDistance, 25,
        XtNvertDistance, 10,
        XtNlabel, "Testing set:",
        NULL);

    trainingSetRB= XtVaCreateManagedWidget("trainingSetRB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, alwaysUseWtsRB,
        XtNfromHoriz, testWeightsLabel,
        XtNvertDistance, 10,
        NULL);

    trainingSetRBLabel= XtVaCreateManagedWidget("trainingSetRBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, alwaysUseWtsRB,
        XtNfromHoriz, trainingSetRB,
        XtNvertDistance, 10,
        XtNhorizDistance, 0,
        XtNlabel, "Training set",
        NULL);

    trainingSetLabel= XtVaCreateManagedWidget("trainingSetLabel",
        labelWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNfromVert, alwaysUseWtsRB,
        XtNfromHoriz, trainingSetRBLabel,
        XtNvertDistance, 10,
        XtNlabel, "<< not available >>",
        NULL);

    novelDataRB= XtVaCreateManagedWidget("novelDataRB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, trainingSetRB,
        XtNfromHoriz, testWeightsLabel,
        NULL);

    novelDataRBLabel= XtVaCreateManagedWidget("novelDataRBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, trainingSetRB,
        XtNfromHoriz, novelDataRB,
        XtNhorizDistance, 0,
        XtNlabel, "Novel data:",
        NULL);

    novelDataText= XtVaCreateManagedWidget("novelDataText",
        asciiTextWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, trainingSetRB,
        XtNfromHoriz, novelDataRBLabel,
        XtNwidth, 150,
        XtNdisplayCaret, 0,
        NULL);

    testSweepsLabel= XtVaCreateManagedWidget("testSweepsLabel",
        labelWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNfromVert, novelDataRB,
        XtNhorizDistance, 20,
        XtNvertDistance, 10,
        XtNlabel, "Test sweeps:",
        NULL);

    autoSweepsRB= XtVaCreateManagedWidget("autoSweepsRB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, novelDataRB,
        XtNfromHoriz, testWeightsLabel,
        XtNvertDistance, 10,
        NULL);

    autoSweepsRBLabel= XtVaCreateManagedWidget("autoSweepsRBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY, 
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, novelDataRB,
        XtNfromHoriz, autoSweepsRB,
        XtNvertDistance, 10,
        XtNhorizDistance, 0,
        XtNlabel, "Auto (1 epoch)",
        NULL);

    testSweepsRB= XtVaCreateManagedWidget("testSweepsRB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, autoSweepsRB,
        XtNfromHoriz, testWeightsLabel,
        NULL);

    testSweepsText= XtVaCreateManagedWidget("testSweepsText",
        asciiTextWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, autoSweepsRB,
        XtNfromHoriz, testSweepsRB,
        XtNhorizDistance, 4,
        XtNwidth, 80,
        XtNdisplayCaret, 0,
        NULL);

    outputToScrCB= XtVaCreateManagedWidget("outputToScrCB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, testSweepsRB,
        XtNhorizDistance, 50,
        XtNvertDistance, 10,
        NULL);

    outputToScrCBLabel= XtVaCreateManagedWidget("outputToScrCBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, testSweepsRB,
        XtNfromHoriz, outputToScrCB,
        XtNhorizDistance, 0,
        XtNvertDistance, 10,
        XtNlabel, "Send output to window",
        NULL);


    outputToFileCB= XtVaCreateManagedWidget("outputToFileCB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, outputToScrCB,
        XtNhorizDistance, 50,
        NULL);

    outputToFileCBLabel= XtVaCreateManagedWidget("outputToFileCBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, outputToScrCB,
        XtNfromHoriz, outputToFileCB,
        XtNhorizDistance, 0,
        XtNlabel, "Append output to file:",
        NULL);

    outputFileText= XtVaCreateManagedWidget("outputFileText",
        asciiTextWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, outputToScrCB,
        XtNfromHoriz, outputToFileCBLabel,
        XtNwidth, 150,
        XtNdisplayCaret, 0,
        NULL);

    testCalcErrorCB= XtVaCreateManagedWidget("testCalcErrorCB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, outputToFileCB,
        XtNhorizDistance, 50,
        XtNvertDistance, 10,
        NULL);

    testCalcErrorCBLabel= XtVaCreateManagedWidget("testCalcErrorCBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, outputToFileCB,
        XtNfromHoriz, testCalcErrorCB,
        XtNhorizDistance, 0,
        XtNvertDistance, 10,
        XtNlabel, "Calculate Error",
        NULL);

    testLogErrorCB= XtVaCreateManagedWidget("testLogErrorCB",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, outputToFileCB,
        XtNfromHoriz, testCalcErrorCBLabel,
        XtNhorizDistance, 15,
        XtNvertDistance, 10,
        NULL);

    testLogErrorCBLabel= XtVaCreateManagedWidget("testLogErrorCBLabel",
        toggleWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNtranslations, testToggleTrans,
        XtNborderWidth, 0,
        XtNfromVert, outputToFileCB,
        XtNfromHoriz, testLogErrorCB,
        XtNhorizDistance, 0,
        XtNvertDistance, 10,
        XtNlabel, "Log Error",
        NULL);

    testCancel= XtVaCreateManagedWidget("trainCancel",  
        commandWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNlabel, " Cancel ",
        XtNfromVert, testCalcErrorCB,
        XtNfromHoriz, outputToScrCBLabel,
        XtNvertDistance, 6,
        XtNhorizDistance, 10,
        NULL);

    testOK= XtVaCreateManagedWidget("trainOK",          
        commandWidgetClass, testOptsForm, LOCK_GEOMETRY,
        XtNborderWidth, 4,
        XtNlabel, "    OK    ",
        XtNfromVert, testCalcErrorCB,
        XtNfromHoriz, testCancel,
        XtNvertDistance, 3,
        XtNhorizDistance, 15,
        NULL);

    XtAddCallback(mTestPane[0], XtNcallback, DialogPopup, (XtPointer)testOptsShell);
    XtAddCallback(testOptsShell, XtNpopupCallback, InitTestingDialog, (XtPointer)statusBar);

    XtAddCallback(testCancel, XtNcallback, CancelTestingDialog, (XtPointer)testOptsShell);
    XtAddCallback(testOK, XtNcallback, OKTestingDialog, (XtPointer)testOptsShell);


    /* Init output window */

    outputShell= XtCreatePopupShell("xtlearn Testing Output",
        topLevelShellWidgetClass, appWidget, NULL, 0);

    outputForm= XtVaCreateManagedWidget("outputForm", 
        formWidgetClass, outputShell,
        XtNheight, DISPLAY_DEFAULT_HEIGHT,
        XtNdefaultDistance, -1,
        NULL);

    outputClose= XtVaCreateManagedWidget("outputClose", 
        commandWidgetClass, outputForm, LOCK_GEOMETRY,
        XtNhorizDistance, 4,
        XtNvertDistance, 2,
        XtNlabel, " Close ",
        NULL);

    outputText= XtVaCreateManagedWidget("outputText",
        asciiTextWidgetClass, outputForm,
        XtNtop, XtChainTop,
        XtNscrollVertical, XawtextScrollWhenNeeded,
        XtNscrollHorizontal, XawtextScrollWhenNeeded,
        XtNeditType, XawtextEdit,
        XtNtranslations, editTextTrans,
        XtNfromVert, outputClose,
        XtNvertDistance, 2,
        XtNwidth, 450,
        XtNheight, 450,
        XtNleftMargin, 5,
        XtNtopMargin, 5,
        NULL);

    XtAddCallback(outputClose, XtNcallback, CloseOutputShell, NULL);

    XtAddCallback(mTrainPane[1], XtNcallback, StartSystem, NULL);
    XtAddCallback(mTrainPane[2], XtNcallback, StartSystem, NULL);
    XtAddCallback(mTestPane[1], XtNcallback, StartSystem, NULL);
    XtAddCallback(mTestPane[2], XtNcallback, StartSystem, NULL);

    XtAddCallback(mDispPane[0], XtNcallback, PopupErrDisplay, NULL);
    XtAddCallback(mDispPane[1], XtNcallback, PopupActDisplay, NULL);
    XtAddCallback(mDispPane[2], XtNcallback, PopupWtsDisplay, NULL);
    XtAddCallback(mDispPane[3], XtNcallback, PopupArchDisplay, NULL);
    XtAddCallback(mDispPane[4], XtNcallback, XtCallbackNone, outputShell);

    XtAddCallback(weightsButton, XtNcallback, X_save_wts, NULL);
    XtAddCallback(abortButton, XtNcallback, AbortRun, NULL);
    XtAddCallback(mTrainPane[3], XtNcallback, AbortRun, NULL);
    XtAddCallback(mTestPane[3], XtNcallback, AbortRun, NULL);


    /* Init displays and AlertBox */

    InitAlertBox(appContext, appWidget);
    InitArchDisplay(appContext, appWidget);
    InitActDisplay(appContext, appWidget);
    InitErrDisplay(appContext, appWidget);
    InitWtsDisplay(appContext, appWidget);


    /*  Now, finally, put the interface up on the screen */


    XtRealizeWidget(appWidget);

    if (argc == 2 && *root) {
        if (LoadOptions(0)) *root= *saveroot= 0;
    }
    else set_all_user_settings();

    ManageTrainOptsWidgets();

    if (root[0]) XtVaSetValues(projName, XtNlabel, (String)root, NULL);

    gcVals.foreground= BlackPixel(appDisplay, 0); /* Create a black on white   */
    gcVals.background= WhitePixel(appDisplay, 0); /* Graphics Context with a   */
    gcVals.stipple= grayStipple;                  /* gray stipple & small font */
    gcVals.font= XLoadFont(appDisplay, "-*-*-medium-r-semicondensed-*-13-*-*-*-c-*-iso8859-*");
    appGC= XCreateGC(appDisplay, appWindow, 
                     GCForeground | GCBackground | GCStipple | GCFont, &gcVals);

    gcVals.foreground= WhitePixel(appDisplay, 0);  /* Create a white on black */
    gcVals.background= BlackPixel(appDisplay, 0);  /* Graphics Context        */
    revGC= XCreateGC(appDisplay, appWindow, GCForeground | GCBackground, &gcVals);

    waitCursor= XCreateFontCursor(appDisplay, XC_watch);
}


void X_save_wts(Widget w, XtPointer client_data, XtPointer call_data)
{
    int save_wts();   /* make compiler happy */

    if (save_wts()) return;

    sprintf(buf, "%s.%ld.wts", root, sweep);
    if (strlen(buf) > XTLMAXPATH) 
        return;  /* Silently fail to set most recent weights file -- too long */
    strcpy(TestOpts.mostRecentFile, buf);
    TestOpts.mostRecentWts= 1;    /* Assume user will want to test with these */
    SaveTestingOptions();
}




int LoadOptions(short run)
{
    int i;
    FILE *prjfile;
    long len;
    short err= 0, gettingLast= 0;
    
    sprintf(buf, "%s.prj", root);
    prjfile= fopen(buf, "r");

    if (prjfile == NULL) {
        FILE *cffile;
        sprintf(buf, "%s.cf", root);
        cffile= fopen(buf, "r");
        if (cffile == NULL) {
            if (AlertBox("There is no configuration file for this project, create project anyway?", 1, "  Create  ", "  Cancel  ", NULL) == 2)
                return -1;
        }
        else fclose(cffile);
        sprintf(buf, "%s.prj", root);
        prjfile= fopen(buf, "w+");
    }

    if (run == 0) {                      /* if run == 0 then get the LAST run */
        gettingLast= 1;                  /* and read in the Testing Opts too. */
        fread(&TestOpts, sizeof(TestOpts), 1, prjfile);
        err= fseek(prjfile, -(sizeof(TrainOpts)), SEEK_END);
        len= ftell(prjfile);
        if (err || len < sizeof(TestOpts)) {  /* prjfile is empty (that's OK) */
            fseek(prjfile, 0L, SEEK_END);
            len= ftell(prjfile);
            fclose(prjfile);
            for (i= 0; i < sizeof(TrainOpts); i++) ((char *)&TrainOpts)[i]= 0;
            TrainOpts.sweeps= 5000;
            TrainOpts.learnRate= 0.1;
            TrainOpts.updateInterval= 1;
            TrainOpts.bpttCopies= 1;
            gLastRun= 0;

            if (len == sizeof(TestOpts)) return 0; 
            for (i= 0; i < sizeof(TestOpts); i++) ((char *)&TestOpts)[i]= 0;
            TestOpts.sweeps= 10;
            TestOpts.mostRecentWts= 1;
            TestOpts.autoSweeps= 1;
            TestOpts.outputDestScr= 1;
            SaveTestingOptions();
            return 0;
        }
            
        gLastRun= run= 1+ (len -sizeof(TestOpts))/sizeof(TrainOpts);
    }

    else fseek(prjfile, sizeof(TestOpts) + (run -1) *sizeof(TrainOpts), SEEK_SET);

    if (fread(&TrainOpts, sizeof(TrainOpts), 1, prjfile) != 1) err= 1;
    fclose(prjfile);
    if (gettingLast) TrainOpts.runName[0]= 0;
    if (!err) return 0;

    return report_condition("Unable to load project file.", 2);
}


int SaveTrainingOptions(short run)
{
    FILE *prjfile;
    short ret;
    
    sprintf(buf, "%s.prj", root);
    prjfile= fopen(buf, "r+");
    if (run == 0) {
        fseek(prjfile, 0L, SEEK_END);
        gLastRun++;
    }

    else fseek(prjfile, sizeof(TestOpts) + (run -1) * sizeof(TrainOpts), SEEK_SET);
    
    ret= fwrite(&TrainOpts, sizeof(TrainOpts), 1, prjfile);
    fclose(prjfile);

    if (ret == 1) return 0;

    return report_condition("Unable to write the project file.", 2);
}

    
int SaveTestingOptions(void)
{
    FILE *prjfile;
    short ret;
    
    sprintf(buf, "%s.prj", saveroot);
    prjfile= fopen(buf, "r+"); 
    if (prjfile == NULL) return report_condition("Unable to open the project file.", 2);
    ret= fwrite(&TestOpts, sizeof(TestOpts), 1, prjfile);
    fclose(prjfile);

    if (ret == 1) return 0;
    return report_condition("Unable to write the project file.", 2);
}
    

static void SetRunWindowTitle(void)
{
    Dimension width;

    sprintf(buf, "\"%s\" run %d of %d", *TrainOpts.runName ? TrainOpts.runName : 
     root, runNumber, gLastRun +1);

    XtVaGetValues(runNameText, XtNwidth, &width, NULL);
    XtVaSetValues(runNameText, XtNlabel, buf, NULL);
    XtVaSetValues(runNameText, XtNwidth, width, NULL);
}


static void ProcessPendingXEvents(void)
{
    XEvent event;

    while (XtAppPending(appContext)) {
        XtAppNextEvent(appContext, &event);
        XtDispatchEvent(&event);
    }
}


static void ProcessXEvent(void)
{
    static XEvent event;
                                                         /* Prevent blocking */
    if (gRunning && !XtAppPending(appContext)) return;   /*  while running   */
                                                 
    XtAppNextEvent(appContext, &event);
    XtDispatchEvent(&event);
}


void MainXEventLoop(void)
{
    time_t thisTime, nextTime= 0;

    while (1) {
        ProcessXEvent();

        if (checkTrainOpts) CheckTrainOpts();
        if (checkTestOpts) CheckTestOpts();
        if (checkSetProj) CheckSetProj();
        if (!gRunning || gPause) continue;

        if (sweep >= nsweeps || gAbort) {
            gRunning= 0;

            UpdateStatusSweeps();
            UpdateStatusText(gAbort? "aborted" : "completed");
            gAbort= 0;
            UpdateStatusWindowButtons();

            if (learning) X_save_wts(0, 0, 0);
            if (probe || verify) {
                if (output_file) fclose(output_file);
                /* was: fclose(output_file); BillW after Damien Fisher Sep 02 */
                output_file= NULL;
            }
            continue;
        }

        sweep++;
        XTlearnLoop();

        thisTime= time(NULL);
        if (thisTime > nextTime) {
            UpdateStatusSweeps();
            nextTime= thisTime;
        }
    }
}



void XReportCondition(char *message, int level)
{
    if (level == 0) {  /* Level zero:  just right to the info area */
        XtVaSetValues(infoArea, XtNstring, message, NULL);
        return;
    }

    if (level > 1) XBell(XtDisplay(appWidget), 0);
    AlertBox(message, level, "    OK    ", 0, 0);
}



static void AboutBox(Widget w, XtPointer client_data, XtPointer call_data)
{
    sprintf(buf, "%s version %s\nby %s, @crl.ucsd.edu %s.\n\n" \
                    "OSXtlearn by Harm Brouwer <me@hbrouwer.eu>\n%s", 
        PROGRAM_NAME, PROGRAM_VERSION, PROGRAMMERS, PROGRAM_DATE,
#ifdef EXP_TABLE
	"\nRequires exp table: "EXP_TABLE);
#else
        "");
#endif

    AlertBox(buf, 0, "     OK     ", 0, 0);
}


int DisplayOutputs(void)
{
    short i;

    for (i = 0; i < no; i++) {
        sprintf(buf, "%7.3f     ", zold[ni +outputs[i]]);
        buf[(strlen(buf)/4) *4]= 0;
        if (TestOpts.outputDestScr && WriteToOutputWindow(buf)) return -1;
        if (TestOpts.outputDestFile && WriteToOutputFile(buf)) return -1;
    }

    if (TestOpts.outputDestScr && WriteToOutputWindow("\n")) return -1;
    if (TestOpts.outputDestFile && WriteToOutputFile("\n")) return -1;
    return 0;
}


int DisplayNodes(void)
{
    int     i;

    for (i = 1; i <= nn; i++) {
        if (!selects[i]) continue;
        sprintf(buf, "%7.3f     ", zold[ni +i]);
        buf[(strlen(buf)/4) *4]= 0;
        if (TestOpts.outputDestScr && WriteToOutputWindow(buf)) return -1;
        if (TestOpts.outputDestFile && WriteToOutputFile(buf)) return -1;
    }

    if (TestOpts.outputDestScr && WriteToOutputWindow("\n")) return -1;
    if (TestOpts.outputDestFile && WriteToOutputFile("\n")) return -1;
    return 0;
}


    /* If the system is simulating, will abort on next sweep. */

static void AbortRun(Widget w, XtPointer client_data, XtPointer call_data)
{
    UpdateStatusSweeps();  /* make sure displays agree */
    gAbort= 1;
    if (!learning) return;
    sprintf(buf, "Save the current weights file %s.%ld.wts before aborting?", root, sweep);
    switch (AlertBox(buf, 2, "    Save    ", "  Cancel  ", " Don't Save ")) {
        case 3:   learning= 0;  /* Prevent weights from being saved */
        case 1:   return;
        case 2:   gAbort= 0;
    }
}

#define MAXHEADER 300

static int OutputWindowHeader(void)
{
    short i, k, len;
    char tmp[MAXHEADER];
    
#ifdef oldoutputheader
    if (probe) {
        strcpy(tmp, "Node ");
        for (i = 1, k= 0; i <= nn; i++) if (selects[i]) k++;
        for (i = 1; i <= nn; i++) {
            if (!selects[i]) continue;
            sprintf((char *)(tmp +strlen(tmp)), "%d", i);
            if (--k <= 0) break;
            strcat(tmp, k == 1 ? " & " : ", ");
        }
    }
    else strcpy(tmp, "Output");
#endif

    if (probe) {
        strcpy(tmp, "Node ");
        for (i = 1, k= 0; i <= nn; i++) if (selects[i]) k++;
        if (k == 0) return report_condition("No nodes were selected for probing.  Add a line to your .cf file under SPECIAL.  e.g. selected = 1-3", 0);
        for (i = 1; i <= nn; i++) {
            if (!selects[i]) continue;
            len= strlen(tmp);
            if (len + 100 > MAXHEADER) {
                if (TestOpts.outputDestScr && WriteToOutputWindow(tmp)) return -1;
                if (TestOpts.outputDestFile && WriteToOutputFile(tmp)) return -1;
                len= 0;
            }
            sprintf((char *)(tmp +len), "%d", i);
            if (--k <= 0) break;
            strcat(tmp, k == 1 ? " & " : ", ");
        }
    }
    else strcpy(tmp, "Output");
   

    sprintf((char *)(tmp +strlen(tmp)), " activations\nusing %s and %s.data\n",
            loadfile, TestOpts.novelData ? TestOpts.novelFile : root);

    if (TestOpts.outputDestScr) {
        if (WriteToOutputWindow(tmp)) return -1;
        XtPopup(outputShell, XtGrabNone);
        XtSetSensitive(mDispPane[4], 0);
    }

    if (TestOpts.outputDestFile)
        if (WriteToOutputFile(tmp)) return -1;
    
    return 0;
}
    


static int WriteToOutputWindow(char *str)
{
    XawTextBlock txtBlock;

    txtBlock.firstPos= 0;
    txtBlock.length= strlen(str);
    txtBlock.ptr= str;
    txtBlock.format= FMT8BIT;
    
    XawTextReplace(outputText, 10000000, 10000000, &txtBlock);
    XawTextSetInsertionPoint(outputText, 10000000); /* 10 Megs oughta do */

    return 0;
}


static int WriteToOutputFile(char *str)
{
    if (output_file == NULL) {
        output_file= fopen(TestOpts.outputFile, "a");
        if (output_file == NULL) {
            sprintf(buf, "Could not open output file \"%s\".", TestOpts.outputFile);
            return report_condition(buf, 1);
        }
    }

    fprintf(output_file, "%s", str);

    return 0;
}

