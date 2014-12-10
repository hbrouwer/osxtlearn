/*
	general_X.h
*/

#ifndef GENERAL_X_H
#define GENERAL_X_H



/* X stuff: */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/AsciiText.h>

#if XtSpecificationRelease < 5
#define repeaterWidgetClass commandWidgetClass
#define STOP_ACTION "reset"
typedef Cardinal ARGCTYPE;
#else
typedef int ARGCTYPE;
#include <X11/Xaw/Repeater.h>
#define STOP_ACTION "stop"
#endif

#define DISPLAY_DEFAULT_HEIGHT 320
#define DISPLAY_DEFAULT_WIDTH 635

#define LOCK_GEOMETRY XtNtop,XtChainTop,XtNbottom,XtChainTop,\
                     XtNleft,XtChainLeft,XtNright,XtChainLeft

#define XTLMAXPATH 80

typedef struct {
    long sweeps;	       /*  number of sweeps used for learning: -s */
    float learnRate;	       /*  learning rate: -r   */
    char fixedSeed;	       /*  if true, seeds random number with userSeed */
    short userSeed;	       /*  the seed value set by user: -S   */
    float momentum;	       /*  momentum: -m   */
    float initialBias;	       /*  initial bias setting: -B   */
    short crossEntropyOpt;     /*  cross-entropy error options: -H   */
    char randPatterns;	       /*  if true, presents teach patterns randomly: -R*/
    char writeError;	       /*  if true, writes error file every errorInterval sweeps */
    long errorInterval;	       /*  sweep interval that error file is written: -E*/
    char dumpWeights;	       /*  If true, weights files are dumped every dumpInterval   */
    long dumpInterval;	       /*  Interval that weights files are written: -C */
    char loadWeights;	       /*  If true, preloads weights file: -l   */
    char weightsFile[XTLMAXPATH]; /*  weights filename   */
    float errCriterion;	       /*  stop running if RMS error falls below this value: -M   */
    short bpttFlag;	       /*  if true, uses back prop through time   */
    short bpttCopies;	       /*  copies used for bptt: -Z   */
    long updateInterval;       /*  sweep interval that weight updates are made: -U    */
    char useResetFile;	       /*  if true, reads reset file for reset times: -X   */
    char teacherForcing;       /*  if true, forces learning...  */
    char runName[40];	       /*  optional name for the run    */
} TrainingOptions;

typedef struct {
    long sweeps;                /*  number of sweeps used for verifying and probing   */
    char weightsFile[XTLMAXPATH]; /*  name of weights file used for verifying and probing   */
    char novelData;             /*  if true, use novelFile for verifying and probing   */
    char novelFile[XTLMAXPATH]; /*  name of novel data file used for verifying and probing   */
    char mostRecentWts;         /*  if true, uses most recent weights for verifying and probing   */
    char mostRecentFile[XTLMAXPATH];  /*  name of most recently written weights file    */
    char autoSweeps;            /*  if true, sets verify sweeps to one epoch  */
    char outputDestScr;         /*  if true, send test output to screen Output window   */
    char outputDestFile;        /*  if true, send test output to testOutFile   */
    char outputFile[XTLMAXPATH];/*  name of file to write output to */
    char bigDialog;             /*  if true, uses the big dialog box for Training Options   */
    char calculateError;        /*  if true, calculate error during testing */
    char writeError;            /*  if true and calculateError is true, write out error */
} TestingOptions;



#endif
