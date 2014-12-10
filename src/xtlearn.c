/* tlearn.c - simulator for arbitrary networks with time-ordered input */
/*------------------------------------------------------------------------

This program simulates learning in a neural network using either standard
back-propagation learning algorithm or back-prop through time.  The input 
is a sequence of vectors of (ascii) floating point numbers contained in a 
".data" file.  The target outputs are a set of vectors of (ascii) floating 
point numbers (including optional "don't care" values) in a ".teach" file. 
The network configuration is defined in a ".cf" file documented in the 
tlearn manual.


BPTT NOTE from David Spitz: 
      Sections of code (throughout tlearn) are isolated by lines
      of asterisks; these sections have been added to tlearn to support
      back-propagation through time, OR these sections contain
      original tlearn code which has been modified to support "bptt".
      In cases of modification, comments should clarify what has
      been changed. For information regarding the implementation of
      "bptt" in tlearn, see the accompanying file named "README-bptt".

------------------------------------------------------------------------*/

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include "general.h"
#include "cli.h"
#include "subs.h"
#include "error.h"
#include "parse.h"
#include "arrays.h"
#include "update.h"
#include "compute.h"
#include "weights.h"
#include "activate.h"
#include "xtlearn.h"

#ifdef XTLEARN
#include "general_X.h"
#include "xtlearnErrDisp.h"
#include "xtlearnWtsDisp.h"
#include "xtlearnIface.h"
extern TestingOptions TestOpts;
extern TrainingOptions TrainOpts;
#endif


int    nn;        /* number of nodes */
int    ni;        /* number of inputs */
int    no;        /* number of outputs */
int    nt;        /* nn + ni + 1 */
int    np;        /* ni + 1 */

CFStruct    **cinfo;   /* (nn x nt) connection info used throughout tlearn */
NFStruct    *ninfo;    /* (nn) node activation funct info used throughout */

int    *outputs;    /* (no) indices of output nodes */
int    *selects;    /* (nn+1) nodes selected for probe printout */
int    *linput;     /* (ni) localist input array */

float    *znew;        /* (nt) inputs and activations at time t+1 */
float    *zold;        /* (nt) inputs and activations at time t */
float    *zmem;        /* (nt) inputs and activations at time t */
float    **wt;         /* (nn x nt) weight TO node i FROM node j*/ 
float    **dwt;        /* (nn x nt) delta weight at time t */
float    **winc;       /* (nn x nt) accumulated weight increment*/
float    *target;      /* (no) output target values */
float    *error;       /* (nn) error= (output - target) values */
float    ***pnew;      /* (nn x nt x nn) p-variable at time t+1 */
float    ***pold;      /* (nn x nt x nn) p-variable at time t */

float    rate= .1;         /* learning rate */
float    momentum= 0.;     /* momentum */
float    weight_limit= 1.; /* bound for random weight init */
float    criterion= 0.;    /* exit program when rms error is less than this */
float    init_bias= 0.;    /* possible offset for initial output biases */

long    sweep= 0;       /* current sweep */
long    tsweeps= 0;     /* total sweeps to date */
long    rms_report= 0;  /* output rms error every "report" sweeps */

int    ngroups= 0;    /* number of groups */

int    teacher= 0;    /* flag for feeding back targets */
int    localist= 0;   /* flag for speed-up with localist inputs */
int    randomly= 0;   /* flag for presenting inputs in random order */
int    limits= 0;     /* flag for limited weights */
int    ce= 0;         /* flag for cross_entropy */

char    root[128];       /* root filename may change during testing */
char    saveroot[128];   /* copy of rootname: NEVER CHANGES */
char    loadfile[128];   /* filename for weightfile to be read in */
FILE    *cfp;            /* file pointer for .cf file */

static void usage(void);
static void intr(int);


extern int getopt(), optind;  /* getopt stuff */


extern    char *optarg;
extern    time_t time();

int gAbort, gRunning;
int batch_mode, interactive;

/* ************************************************ */
int    bptt;       /* "bptt" - flag indicating back-prop through time; */
int    copies= 2;  /* copies indicates how many time steps to simulate */
                      /* if we're using bptt. */
int    true_nn;    /* Used for bptt; since unfolding the network */
int    true_ni;    /* will alter the actual n_ counts, we'll use */
int    true_no;    /* these to store the originals for later use. */
int    true_nt;
int    true_np;

float    *copy_w;    /* Used for weight matrix transformation. */

/* ************************************************ */

int    learning= 1;    /* flag for learning */
int    reset= 0;    /* flag for resetting net */
int    verify= 0;    /* flag for printing output values */
int    probe= 0;    /* flag for printing selected node values */
int    resume= 0;
int    loadflag= 0;    /* flag for loading initial weights from file */
int    rflag= 0;    /* flag for -X */
int    seed= 0;    /* seed for random() */
int    fixedseed= 0;  /* flag for random seed fixed by user */

long    nsweeps= 0;    /* number of sweeps to run for */
long    tmax= 0;    /* maximum number of sweeps (parsed from .data) */
long    umax= 0;    /* update weights every umax sweeps */
long    check= 0;    /* output weights every "check" sweeps */

float    *w;
float    *wi;
float    *dw;
float    *pn;
float    *po;

CFStruct    *ci;

static float    err= 0.;    /* cumulative ss error */
static float    ce_err= 0.;    /* cumulate cross_entropy error */

static long    ttime= 0;    /* number of sweeps since time= 0 */
static long    utime= 0;    /* number of sweeps since last update_weights */
static long    rtime= 0;    /* number of sweeps since last rms_report */
static long    chktm= 0;    /* number of sweeps since last check */
static int    command= 1;    /* flag for writing to .cmd file */
static char cfile[128];    /* filename for .cf file */


static char buf[128];


int main(int argc, char **argv)
{
    int i, c;
    FILE *cmdfp;

    signal(SIGINT, intr);  /* Trap these signals and send them to intr */
    signal(SIGHUP, intr);
    signal(SIGQUIT, intr);
    signal(SIGKILL, intr);

    if (!strcmp(argv[0], "tlearn")) batch_mode= 1;

    while ((c= getopt(argc, argv, "BIZ:f:hil:m:r:s:tC:E:LM:PpRS:TU:VvXO:H:D:")) != EOF) {

        switch (c) {

/* ********************************************* */
            case 'Z':    /* bptt flag; optarg is number of copies */
                bptt= 1;      
                copies= atoi(optarg);  
                break;
/* ********************************************* */
            case 'C':
                check= (long) atol(optarg);
                chktm= check;
                break;
            case 'f':
                strcpy(root, optarg);
                break;
            case 'i':
                command= 0;
                break;
            case 'l':
                loadflag= 1;
                strcpy(loadfile, optarg);
                break;
            case 'm':
                momentum= (float) atof(optarg);
                break;
            case 'P':
                learning= 0;
                /* drop through deliberately */
            case 'p':
                probe= 1;
                break;
            case 'r':
                rate= (float) atof(optarg);
                break;
            case 's':
                nsweeps= (long) atol(optarg);
                break;
            case 't':
                teacher= 1;
                break;
            case 'V':
                learning= 0;
                /* drop through deliberately */
            case 'v':
                verify= 1;
                break;
            case 'X':
                rflag= 1;
                break;
            case 'E':
                rms_report= (long) atol(optarg);
                break;
            case 'I':
                batch_mode= 0;
                interactive= 1;
                break;
            case 'B':
                batch_mode= 1;
                interactive= 0;
                break;
            case 'M':
                criterion= (float) atof(optarg);
                break;
            case 'R':
                randomly= 1;
                break;
            case 'S':
                fixedseed= 1;
                seed= atoi(optarg);
                break;
            case 'U':
                umax= atol(optarg);
                break;
            case 'O':
                init_bias= atof(optarg);
                break;
            /*
             * if == 1, use cross-entropy as error;
             * if == 2, also collect cross-entropy stats.
             */
            case 'H':
                ce= atoi(optarg);
                break;
            case '?':
            case 'h':
            default:
                usage();
                exit(2);
                break;
        }
    }

    if (!*root && optind +1 == argc) /* If root wasn't given w/ -f and there's */
        strcpy(root, argv[optind]);  /* a token remaining; assume its the root */

    strcpy(saveroot, root); 

    if (exp_init()) return -1;  /* Does nothing if EXP_TABLE is undefined */

#ifdef XTLEARN

    if (!interactive && !batch_mode) {  /* Normal X mode */
        if (XOpenDisplay(NULL)) {
            InitXInterface(argc, argv);
            MainXEventLoop();          /* This function never returns */
        }

        else {
            printf("Could not open display; proceeding in interactive mode...\n");
            interactive= 1;
        }
    }

#endif

    if (!interactive) {
        batch_mode= 1;
        if (root[0] == 0) {
            interactive= 1;
            batch_mode= 0;
            report_condition("No fileroot supplied - invoking interactive command line.", 1);
        }

        if (nsweeps < 1) {
            interactive= 1;
            batch_mode= 0;
            report_condition("Sweeps not specified - invoking interactive command line.", 1);
        }
    }

    if (batch_mode) {         /* In batch mode, so save options in .cmd file */
        sprintf(buf, "%s.cmd", root);
        cmdfp= fopen(buf, "a");
        if (cmdfp == NULL) {
            sprintf(buf, "Could not open \"%s.cmd\" command file.", root);
            report_condition(buf, 1);
        }
        else {
            for (i= 1; i < argc; i++) fprintf(cmdfp, "%s ", argv[i]);
            fprintf(cmdfp, "\n");
            fflush(cmdfp);
            fclose(cmdfp);
        }

        if (set_up_simulation()) exit(1);
        nsweeps += tsweeps;
        sweep= tsweeps;

        while (nsweeps > sweep && !gAbort) {  /* actually run tlearn here */
            if (TlearnLoop()) exit(1);
            sweep++;
        }

        if (learning) 
            if (save_wts()) exit(1);

        exit(0);

      }

    else {    /* Must be interactive. */
        if (init_cli(argc, argv) == ERROR) return ERROR;
        if (run_cli() == ERROR) return ERROR;
        else return NO_ERROR;
    }
}



    /* Here is the old main of tlearn, where, given a set of parameters
       and files which are already open, sets up the network. The
       sweeps are not run here, only the set up. */

int set_up_simulation(void)
{
    int i, j, k;

    err= 0.;
    ce_err= 0.;
    tsweeps= 0;
/*    weight_limit= 1.;*/
    sweep= 0;
/*    ngroups= 0;*/
/*    localist= 0;*/
/*    limits= 0;*/
    ttime= 0;
    utime= 0;
/*    tmax= 0;*/
    rtime= 0;
    chktm= 0;

    if (init_config()) return ERROR;

    if (!fixedseed) { 
        srand((int)time(NULL));
        seed= rand();
    }
    srand(seed);

    if (loadflag) {
        if (load_wts()) return ERROR;
    }
    else {
        for (i= 0; i < nn; i++) {
            w= *(wt + i);
            dw= *(dwt+ i);
            wi= *(winc+ i);
            ci= *(cinfo+ i);
            for (j= 0; j < nt; j++, ci++, w++, wi++, dw++) {
                if (ci->con) 
                    *w= rans(weight_limit);
                else
                    *w= 0.;
                *wi= 0.;
                *dw= 0.;
            }
        }
        /*
         * If init_bias, then we want to set initial biases
         * to (*only*) output units to a random negative number.
         * We index into the **wt to find the section of receiver
         * weights for each output node.  The first weight in each
         * section is for unit 0 (bias), so no further indexing needed.
         */
        for (i= 0; i < no; i++) {
            w= *(wt + outputs[i] - 1);
            ci= *(cinfo + outputs[i] - 1); 
            if (ci->con) 
                *w= init_bias + rans(.1);
            else
                *w= 0.;
        }
    }
/* ********************************************* */
            /* At this point, a fully feed forward
               network exists in Tlearn with randomized
               weights. However, because we must eventually
               refold the network into its original form, 
               each initial weight must be identical to
               its copies. So, we need to duplicate the
               weight matrix in exactly the same manner as
               we duplicated the connection matrix. So, we
               must duplicate the bias, input, and node
               weights to each copy from the initial copy.
               For a more detailed commentary of what's about
               to go on, look at the bptt connection matrix
               transformation (now in parse.c). */
    
    if (bptt) {
            /* Bias duplication */
        for (i= 0; i < true_nn; i++) {
            w= *(wt + i);
            for (j= 1; j < copies; j++) {  /* This is our copy target */
                copy_w= *(wt + i + (j*true_nn));
                    /* ie, one duplication of each bias connection
                       for each copy. */
                *copy_w= *w;
            }
        }
            /* Input duplication */
        for (i= 0; i < true_nn; i++) {
            w= *(wt + i);
            for (j= 1; j <= true_ni ; j++) {
                w++;
                /* get each node's input matrix. */
                for (k= 1; k < copies; k++) {
                        /* get to correct copy... */
                    copy_w= *(wt + i + (k*true_nn));
                        /* get to correct input connection... */
                    copy_w= copy_w + j;
                        /* offset copy's matrix... */
                    copy_w= copy_w + (k * true_ni);

                    *copy_w= *w;
                }
            }
        }
            /* Node duplication */
        for (i= 1; i < copies; i++) {
            int end_node;
            if (i == (copies-1)) end_node= true_nn;
            else end_node= 2 * true_nn;
            for (j= 0; j < end_node; j++) {
                w= *(wt + j);
                w= w + ni + 1;
                copy_w= *(wt + j + (i*true_nn));
                copy_w= copy_w + (i*true_nn) + ni + 1;
                for (k= 0; k < true_nn; k++) {
                    *copy_w= *w;
                    w++;
                    copy_w++;
                }
            }
        }

    }

/* ********************************************* */

    zold[0]= znew[0]= 1.;
    for (i= 1; i < nt; i++)
        zold[i]= znew[i]= 0.;


        /* Here, we read in input, target, and reset files: */


        
    if (!interactive && !batch_mode) { /* Normal X mode */
#ifdef XTLEARN
        if (learning) {
            if (rflag && init_reset()) return -1;
            if (init_inputs(&tmax)) return -1;
        
            if (init_targets()) return -1;
            nsweeps += tsweeps;
            sweep= tsweeps;

            if (AllocErrDisplay()) return -1;
        }

        else if (probe || verify) { /* Setup for Testing */
            if (TestOpts.novelData)
                strcpy(root, TestOpts.novelFile);

            if (rflag && init_reset()) return -1;
            if (init_inputs(&tmax)) return -1;

            if (TestOpts.calculateError && init_targets())
                return report_condition("During testing, the .teach file is only used for calculating error.  If you aren't interested in the error during testing, you may turn it off.  See Testing Options...", 1);

            if (TestOpts.novelData) strcpy(root, saveroot); /* Need to reset - temporarilly changed up above*/
            tsweeps= sweep= 0;
            if (!TestOpts.autoSweeps && TestOpts.sweeps > 0L) 
                nsweeps= TestOpts.sweeps;
            else {
                TestOpts.autoSweeps= 1;
                nsweeps= tmax;
            }

            if (TestOpts.calculateError) {
         /*       rms_report= 1;*/
                if (AllocErrDisplay()) return -1;
         /*       rms_report= 0;*/
            }

        }
#endif

        return 0;
    }

    else {
        if (init_inputs(&tmax)) return -1;
        if (init_targets()) return -1;
        if (reset) if (init_reset()) return -1;
        if (learning) save_wts();

        return 0;
    }
}


int TlearnLoop(void)        /* used for interactive and batch modes */
{
    if (rflag) update_reset(ttime, 0, rflag, &tmax, &reset);
    if (reset) reset_bp_net(zold, znew, zmem);

    if (update_inputs(zold, 0, 0, &tmax, &linput)) return ERROR;

    if (learning || teacher || rms_report)
        if (update_targets(target, ttime, 0, 0, &tmax)) return ERROR;

    if (act_nds(zold, zmem, znew, wt, linput, target)) return ERROR;

    comp_errors(zold, target, error, &err, &ce_err);

    if (learning) comp_backprop(wt, dwt, zold, zmem, target, error, linput);

    if (probe) print_nodes(zold);
    if (verify) print_output(zold);

    if (rms_report && (++rtime >= rms_report)) {
        rtime= 0;
        print_error((ce == 2) ? &ce_err : &err, sweep);
    }

    if (check && learning && (++chktm >= check)) {
        chktm= 0;
        if (save_wts()) return ERROR;
    }

    if (++ttime >= tmax) ttime= 0;

    if (learning && (++utime >= umax)) {
        utime= 0;
        if (update_weights(wt, dwt, winc)) return ERROR;
    }

    return NO_ERROR;
}


#ifdef XTLEARN

extern int errInterval;
extern void PlotErrPoint(float *err);

int XTlearnLoop(void)         /* Used for X Window System mode */
{
    if (rflag) update_reset(ttime, 0, rflag, &tmax, &reset);
    if (reset) reset_bp_net(zold, znew, zmem);

    if (update_inputs(zold, 0, 0, &tmax, &linput)) return -1;

/*    if (learning || teacher || rms_report || ((probe || verify) && TestOpts.calculateError))*/
    if (!(probe || verify) || TestOpts.calculateError)    
        if (update_targets(target, ttime, 0, 0, &tmax)) return -1;

    if (act_nds(zold, zmem, znew, wt, linput, target)) return -1;

    if (!(probe || verify) || TestOpts.calculateError)    
      comp_errors(zold, target, error, &err, &ce_err);

    if (learning) comp_backprop(wt, dwt, zold, zmem, target, error, linput);

    if (verify && DisplayOutputs()) return -1;
    if (probe && DisplayNodes()) return -1;

    if (!(probe || verify) || TestOpts.calculateError)    
      if (++rtime >= errInterval) {
        rtime= 0;
        PlotErrPoint((ce == 2) ? &ce_err : &err);
      }

    AnimateWeightsDisplay();

    if (check && learning && (++chktm >= check)) {
        chktm= 0;
        X_save_wts(0, 0, 0);
    }

    if (++ttime >= tmax) ttime= 0;
    if (learning && (++utime >= umax)) {
        utime= 0;
        update_weights(wt, dwt, winc);
    }

    return 0;
}

#endif

static void usage(void) 
{
    fprintf(stderr, "\n");
    fprintf(stderr, "-f fileroot:\tspecify fileroot <always required>\n");
    fprintf(stderr, "-l weightfile:\tload in weightfile\n");
    fprintf(stderr, "-s #:\trun for # sweeps <always required>\n");
    fprintf(stderr, "-r #:\tset learning rate to # (between 0. and 1.) [0.1]\n");
    fprintf(stderr, "-m #:\tset momentum to # (between 0. and 1.) [0.0]\n");
    fprintf(stderr, "-t:\tfeedback teacher values in place of outputs\n");
    fprintf(stderr, "-Z #:\tactivate BPTT with # copies\n");
    fprintf(stderr, "-S #:\tseed for random number generator [random]\n");
    fprintf(stderr, "-U #:\tupdate weights every # sweeps [1]\n");
    fprintf(stderr, "-E #:\trecord rms error in .err file every # sweeps [0]\n");
    fprintf(stderr, "-C #:\tcheckpoint weights file every # sweeps [0]\n");
    fprintf(stderr, "-M #:\texit program when rms error is less than # [0.0]\n");
    fprintf(stderr, "-X:\tuse auxiliary .reset file\n");
    fprintf(stderr, "-P:\tprobe selected nodes on each sweep (no learning)\n");
    fprintf(stderr, "-V:\tverify outputs on each sweep (no learning)\n");
    fprintf(stderr, "-R:\tpresent input patterns in random order\n");
    fprintf(stderr, "-I:\tInvoke interactive command-line interface\n");
    fprintf(stderr, "-B:\tRun in batch mode - no graphics.\n");
    fprintf(stderr, "-O #:\toffset for offset bias initialization\n");
    fprintf(stderr, "Note: If you tried to pass a standard X command line\n");
    fprintf(stderr, "      option (like -bg blue), and it failed, you are\n");
    fprintf(stderr, "      not using an X terminal, so such options are invalid.\n");
#ifdef EXP_TABLE
    fprintf(stderr, "\nRequires exp table: "EXP_TABLE"\n");
#endif
}


static void intr(int sig)
{
    if (gRunning) gAbort= 1;    /* If net is running, Ctrl-C aborts the run */
    else exit(1);               /* otherwise it exits */
}



int init_config(void)
{
    static long modtime;
    static int saveBpttFlag, saveBpttCopies;

    if (gRunning) return 0;  /* Silently fail if already running */

    if (!batch_mode && !interactive) strcpy(root, saveroot);
    sprintf(cfile, "%s.cf", root);


#ifdef XTLEARN
    if (!batch_mode && !interactive) {
        report_condition(0, 0); /* blank the info area */
        get_all_user_settings();

    /*
     * check .cf file timestamp and some other cfg vars to see if they 
     * have changed.  If not, don't bother setting up the config again.
     */
        if (modtime == GetFileModTime(cfile)
            && saveBpttFlag == TrainOpts.bpttFlag  
            && saveBpttCopies == TrainOpts.bpttCopies) return 0;
    }
#endif

    if (cfp) fclose(cfp);
    cfp= fopen(cfile, "r");
    if (cfp == NULL) {
        sprintf(buf, "Could not find \"%s\"; check file root.", cfile);
        return report_condition(buf, 2);
    }

    parse_err_set(cfp, cfile, "Configuration");
    if (get_nodes() == ERROR) return ERROR;

/* ************************************************ */
        /* Here, we'll fix the numbers so that we can fake Tlearn into
           expecting a large feed-forward network. */
    if (bptt) {
        true_nn= nn;
        true_ni= ni;
        true_no= no;
        true_nt= nt;
        true_np= np;
        nn= nn*copies;
        ni= ni*copies;
        no= no*copies;    /* Used for output windowing. */
        nt= nn+ni+1;    /* same as original definition. */
        np= ni+1;    /* same as original definition, too. */
    }

/* ************************************************ */
/*    free_arrays();*/
    if (init_compute()) return -1;
    if (make_arrays()) return -1;
    if (get_outputs()) return -1;
    if (get_connections()) return -1;
    if (get_special()) return -1;

/* ************************************************ */
    if (bptt) {
            /* The connection matrix is now fully configured
               for the feed-forward network. As the final step
               towards fooling Tlearn into believing it's got
               a feed-forward network, we need to duplicate
               the ninfo structure so that all of the nodes
               in the network are accounted for. (Currently, 
               it still only describes the first true_nn
               nodes). */
        int i, j;

        for (i= 0; i < true_nn; i++) {
            for (j= 1; j < copies; j++) {
                ninfo[(j*true_nn)+i].func= ninfo[i].func;
                ninfo[(j*true_nn)+i].dela= ninfo[i].dela;
            }
        }
    }    
/* ************************************************ */

    /*
     * the config setup was successful.  Set modtime etc so we don't
     * need to do it again if they don't change.
     */

#ifdef XTLEARN
    if (!batch_mode && !interactive) {
        modtime= GetFileModTime(cfile); 
        saveBpttFlag= TrainOpts.bpttFlag;
        saveBpttCopies= TrainOpts.bpttCopies;
    }
#endif

    return 0;

}


#include <sys/stat.h>
#include <unistd.h>

long GetFileModTime(char *fname)
{
    struct stat filestat;

    stat(fname, &filestat);
    return (long)filestat.st_mtime;
}
