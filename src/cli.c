/*


        cli.c

Synopsis:
        Interactive command-line interface module for
        xtlearn.

General Description:
        These functions support the CLI which is invoked
        vias the -I option. Standard commands are defined
        in cli.h. This CLI supports command aliasing and
        piping to shell. In essense, this CLI is a pseudo-
        shell, with the calling shell as the parent.

        xtlearn searches for the file ".xtlearn_init" first
        in the calling directory, then in the user's
        home directory, and executes any commands therein
        before prompting the user for input. Typically this
        is used to set up a user's alias preferences.


*/

#include <stdio.h>
#include <stdlib.h>

#include "general.h"
#include "error.h"
#include "arch.h"
#include "general_X.h"
#include "xtlearn.h"
#include "weights.h"
#include "cli.h"


#define MAX_HISTORY 50

        /* Command ID's: */
#define CLI_EMPTY_ID            -128
#define CLI_UNKNOWN_ID          -127
#define CLI_QUIT_ID             1
#define CLI_VERSION_ID          2
#define CLI_LOAD_ID             3
#define CLI_SAVE_ID             4
#define CLI_LAST_ID             5
#define CLI_NEXT_ID             6
#define CLI_DEFAULT_ID          7
#define CLI_ERROR_ID            8
#define CLI_ARCHITECTURE_ID     9
#define CLI_ACTIVATIONS_ID      10
#define CLI_WEIGHTS_ID          11
#define CLI_TRAIN_ID            12
#define CLI_VERIFY_ID           13
#define CLI_PROBE_ID            14
#define CLI_HELP_ID             15
#define CLI_SET_ID              16
#define CLI_ALIAS_ID            17
#define CLI_REPEAT_ID           18
#define CLI_HISTORY_ID          19
#define CLI_STATUS_ID           20
#define CLI_FILEROOT_ID         21
#define CLI_WEIGHTS_FILE_ID     22
#define CLI_SWEEPS_ID           23
#define CLI_WEIGHTS_MOMENTUM_ID 24
#define CLI_LEARNING_RATE_ID    25
#define CLI_ERROR_REPORT_ID     26
#define CLI_ERROR_STOP_ID       27
#define CLI_WEIGHTS_UPDATE_ID   28
#define CLI_WEIGHTS_SAVE_ID     29
#define CLI_BIAS_OFFSET_ID      30
#define CLI_BPTT_ID             31
#define CLI_SEED_ID             32
#define CLI_RESET_FILE_ID       33
#define CLI_RANDOM_ORDERING_ID  34
#define CLI_TEACHER_FORCING_ID  35
#define CLI_SHELL_ID            36
#define CLI_ERROR_TYPE_ID       37


#define MAX_ARGUMENTS 64
#define MAX_WORD_LENGTH 64
#define MAX_LINE_LENGTH 512
#define MAX_ARGUMENT_LENGTH 128
#define MAX_COMMANDS 128
        /* including aliases... */

#define DELIMITER        "      \n\r"

typedef struct {
        char word[MAX_WORD_LENGTH];
        int action_id;
        char alias[MAX_WORD_LENGTH];
} Command;

#define NO_ALIAS        ""
#define NO_WORD         ""
#define CLI_ERROR_POINTS 40

#define INIT_FILE       ".xtlearn_init"


extern  long    nsweeps;        /* number of sweeps to run for */
extern  long    umax;   /* update weights every umax sweeps */
extern  long    check;  /* output weights every "check" sweeps */

extern  int     learning;       /* flag for learning */
extern  int     verify; /* flag for printing output values */
extern  int     probe;  /* flag for printing selected node values */
extern  int     loadflag;       /* flag for loading initial weights from file */
extern  int     rflag;  /* flag for -x */
extern  int     seed;   /* seed for random() */
extern  int     fixedseed;  /* flag for random seed fixed by user */

extern  float   rate;   /* learning rate */
extern  float   momentum;       /* momentum */

extern  float   criterion;      /* exit program when rms error is less than this */
extern  float   init_bias;      /* possible offset for initial output biases */

extern  long    sweep;  /* current sweep */
extern  long    tsweeps;        /* total sweeps to date */
extern  long    rms_report;     /* output rms error every "report" sweeps */

extern  int     teacher;        /* flag for feeding back targets */
extern  int     randomly;       /* flag for presenting inputs in random order */
extern  int     ce;             /* flag for cross_entropy */

extern  char    root[128];      /* root filename for .cf, .data, .teach, etc.*/
extern  char    loadfile[128];  /* filename for weightfile to be read in */

extern  int     copies;
extern  int     bptt;


extern int gAbort, gRunning;

extern int system();   /* should be in stdlib.h, but isn't always... */

char
        prompt[128], 
        cli_input_line[MAX_HISTORY][MAX_LINE_LENGTH], 
        **tokens;

int 
        current_input= 0, 
        current_token= 0, 
        file_input= 1, 
        total_commands= 0;

FILE 
        *alias_file= NULL;

Command
        *commands;

static int prepare_cli(char *program_name);
static int prepare_cli(char *program_name);
static void change_command(Command command, char *word, int action, char *alias);
static void add_command(char *word, int action, char *alias);
static void lex(char *line);
static Command retrieve_command(char *word);
static Command unalias(char *word);
static char *get_next_word(void);
static int figure_out(char *token);
static int get_next_command(void);
static void print_command(Command command);
static void cli_help(void);
static void cli_exit_from_tlearn(void);
static void show_status(void);
static void cli_restore_default_settings(void);
static void cli_display_info(void);
static int cli_run_system(void);


        /* Initializes the command line interface. */
int init_cli(int argc, char **argv)
{
    report_condition("Interactive command line mode - \"help\" for help.", 0);
    return prepare_cli(argv[0]);
}


        /* Prepares the cli - loads in all of the available commands. */
static int prepare_cli(char *program_name)
{
    char message[128];
    char file_location1[128];
    char file_location2[128];

    int i= 0;

    /* Set prompt: */
    sprintf(prompt, "%s > ", program_name);

    /* Allocate our parse variables: */
    if ((tokens= (char**)calloc(MAX_ARGUMENTS, sizeof(char*))) == NULL)
        return report_condition("tokens calloc failed.", 3);
    for(i=0;i < MAX_ARGUMENTS;i++) {
        if ((tokens[i]=(char*)calloc(MAX_WORD_LENGTH, sizeof(char))) == NULL)
            return report_condition("tokens[] calloc failed.", 3);
    }

    /* Prepare command array. */
    if ((commands= (Command*)calloc(MAX_COMMANDS, sizeof(Command))) == NULL) {
        return report_condition("commands calloc failed.", 3);
    }


    /* Search for and open initialization file: */
    sprintf(file_location1, "./%s", INIT_FILE);
    if (getenv("HOME") != NULL) {
        sprintf(file_location2, (char*)getenv("HOME"));
    }
    strcat(file_location2, "/"); strcat(file_location2, INIT_FILE);
    if ((alias_file= fopen(file_location1, "r")) == NULL) {
        if ((alias_file= fopen(file_location2, "r")) == NULL) {
            file_input= 0;
        }
        else strcpy(file_location1, file_location2);
    }
    if (file_input) {
        sprintf(message, "Reading initialization file: \"%s\"", file_location1);
        report_condition(message, 0);
    }


    /* Install our commands. */
    add_command(NO_WORD,            CLI_UNKNOWN_ID,             NO_ALIAS);
    add_command("quit",             CLI_QUIT_ID,                NO_ALIAS);
    add_command("version",          CLI_VERSION_ID,             NO_ALIAS);
/*    add_command("load",             CLI_LOAD_ID,                NO_ALIAS);*/
/*    add_command("save",             CLI_SAVE_ID,                NO_ALIAS);*/
/*    add_command("next",             CLI_NEXT_ID,                NO_ALIAS);*/
/*    add_command("last",             CLI_LAST_ID,                NO_ALIAS);*/
/*    add_command("default",          CLI_DEFAULT_ID,             NO_ALIAS);*/
/*    add_command("error",            CLI_ERROR_ID,               NO_ALIAS);*/
/*    add_command("architecture",     CLI_ARCHITECTURE_ID,        NO_ALIAS);*/
/*    add_command("activations",      CLI_ACTIVATIONS_ID,         NO_ALIAS);*/
/*    add_command("weights",          CLI_WEIGHTS_ID,             NO_ALIAS);*/
    add_command("train",            CLI_TRAIN_ID,               NO_ALIAS);
    add_command("verify",           CLI_VERIFY_ID,              NO_ALIAS);
    add_command("probe",            CLI_PROBE_ID,               NO_ALIAS);
    add_command("help",             CLI_HELP_ID,                NO_ALIAS);
/*    add_command("set",              CLI_SET_ID,                 NO_ALIAS);*/
    add_command("alias",            CLI_ALIAS_ID,               NO_ALIAS);
/*    add_command("repeat",           CLI_REPEAT_ID,              NO_ALIAS);*/
    add_command("history",          CLI_HISTORY_ID,             NO_ALIAS);
    add_command("show_parameters",  CLI_STATUS_ID,              NO_ALIAS);
    add_command("shell",            CLI_SHELL_ID,               NO_ALIAS);
    add_command("file_root",        CLI_FILEROOT_ID,            NO_ALIAS);
    add_command("weights_file",     CLI_WEIGHTS_FILE_ID,        NO_ALIAS);
    add_command("sweeps",           CLI_SWEEPS_ID,              NO_ALIAS);
    add_command("weights_momentum", CLI_WEIGHTS_MOMENTUM_ID,    NO_ALIAS);
    add_command("learning_rate",    CLI_LEARNING_RATE_ID,       NO_ALIAS);
    add_command("error_type",       CLI_ERROR_TYPE_ID,          NO_ALIAS);
    add_command("error_report",     CLI_ERROR_REPORT_ID,        NO_ALIAS);
    add_command("error_stop",       CLI_ERROR_STOP_ID,          NO_ALIAS);
    add_command("weights_update",   CLI_WEIGHTS_UPDATE_ID,      NO_ALIAS);
    add_command("weights_save",     CLI_WEIGHTS_SAVE_ID,        NO_ALIAS);
    add_command("bias_offset",      CLI_BIAS_OFFSET_ID,         NO_ALIAS);
    add_command("bptt",             CLI_BPTT_ID,                NO_ALIAS);
    add_command("seed",             CLI_SEED_ID,                NO_ALIAS);
    add_command("reset_file",       CLI_RESET_FILE_ID,          NO_ALIAS);
    add_command("random_ordering",  CLI_RANDOM_ORDERING_ID,     NO_ALIAS);
    add_command("teacher_forcing",  CLI_TEACHER_FORCING_ID,     NO_ALIAS);

    return NO_ERROR;
}





        /* Modify a command (i.e., change its alias). */
static void change_command(Command command, char *word, int action, char *alias)
{
    if (word == NULL) {
        report_condition("text for command modify == NULL!", 3);
        return;
    }
    strcpy(command.word, word);
    command.action_id= action;

    if (alias != NULL) strcpy(command.alias, alias);
    else strcpy(command.alias, NO_ALIAS);
}


        /* Install a command. */
static void add_command(char *word, int action, char *alias)
{
    if (word == NULL) {
        report_condition("text for command add == NULL!", 3);
        return;
    }
    strcpy(commands[total_commands].word, word);
    commands[total_commands].action_id= action;

    if (alias != NULL) strcpy(commands[total_commands].alias, alias);
    else strcpy(commands[total_commands].alias, NO_ALIAS);

    total_commands++;
}



        /* Lexically analyze a line and break it into tokens. */
static void lex(char *line)
{
    int i= 0;
    char *token;
    char message[128];
    char copy[MAX_LINE_LENGTH];

    strcpy(copy, line);
    token= (char*)strtok(copy, DELIMITER);
    while(token != NULL) {
        strcpy(tokens[i++], token);
        token= (char*)strtok(NULL, DELIMITER);
        if (i > MAX_ARGUMENTS && token != NULL) {
            sprintf(message, "Too many words; last word read: %s", token);
            report_condition(message, 1);
            break;
        }
    }


    strcpy(tokens[i], NO_WORD);
    current_token= 0;

}


        /* Return the command associated with the name "word". */
static Command retrieve_command(char *word)
{
    int i;

    if (word == NULL) {
        report_condition("NULL text to retrieve_command!", 2);
        return commands[0];
    }

    for(i= 1; i < total_commands; i++) {
        if (strcmp(word, commands[i].word) == 0) {
            return commands[i];
        }
    }

    return commands[0];
}


        /* Unalias a command until no more unaliasing is possible. */
static Command unalias(char *word)
{
    int i;

    if (word == NULL) {
        report_condition("NULL text to unalias!", 2);
        return commands[0];
    }
    for(i=1;i < total_commands;i++) {
        if (strcmp(word, commands[i].word) == 0) {
            if (strcmp(commands[i].alias, NO_ALIAS) != 0) {
                return unalias(commands[i].alias);
            }
            else {
                return commands[i];
            }
        }
    }
    return commands[0];
}


        /* Retrieve the next word in the token list, or
           prompt the user for more input. */
static char *get_next_word(void)
{
    char temp[64];
    if (current_token < MAX_ARGUMENTS) {
        strcpy(temp, tokens[current_token]);
        strcpy(tokens[current_token], NO_WORD);
        current_token++;
        return temp;
    }

    return NO_WORD;
}


        /* Associate the action associated with a given token. */
static int figure_out(char *token)
{
    char cli_command[MAX_ARGUMENT_LENGTH];
    Command current_command;
    char shell_line[512];
    char word1[64];

    if (strcmp(token, NO_WORD) == 0) return CLI_UNKNOWN_ID;
    if (token != NULL) {
        strcpy(cli_command, token);
        current_command= unalias(cli_command);
        if (current_command.action_id == CLI_UNKNOWN_ID) {
            strcpy(shell_line, NO_WORD);
            strcpy(word1, token);
            while(strcmp(word1, NO_WORD) != 0) {
                strcat(shell_line, word1);
                strcat(shell_line, " ");
                strcpy(word1, get_next_word());
            }
            printf("Issuing to shell: %s\n", shell_line);
            system(shell_line);
        }
        return current_command.action_id;
    }
    return CLI_EMPTY_ID;
}


        /* Retrieve the next command from either the user or
           the initialization file. */
static int get_next_command(void)
{
    char word[64];
    char line[512];

    strcpy(word, get_next_word());
    if (strcmp(word, NO_WORD) == 0) {
        if (!file_input) {
            printf("%s", prompt);
            gets(line);
        }
        else if ((fgets(line, MAX_LINE_LENGTH, alias_file)) ==  NULL) {
            file_input= 0;
            return CLI_EMPTY_ID;
        }
        /* Skip comments... */
        if (file_input&&(line[0] == '#')) {
            strcpy(line, NO_WORD);
            strcpy(word, NO_WORD);
            return CLI_EMPTY_ID;
        }
        else {
            strcpy(cli_input_line[current_input], line);
            if (current_input>MAX_HISTORY) current_input=0;
            lex(cli_input_line[current_input]);
            strcpy(word, get_next_word());
            return figure_out(word);
        }
    }
    else return figure_out(word);
}



        /* Pseudo event-loop, runs the command line interface. */
int run_cli(void)
{
    int command_id;
    Command new_command;
    int i;
    char message[128];
    char shell_line[512];
    char word1[64], word2[64];

    printf("Welcome to xtlearn.\n");

    while(1) {
        command_id= get_next_command();
        switch(command_id) {
            case CLI_REPEAT_ID:
                break;
            case CLI_HISTORY_ID:
                for(i= current_input-1; i>0; i--) 
                    printf("%s\n", cli_input_line[i]);

                for(i= MAX_HISTORY -1; i > current_input; i--)
                    printf("%s\n", cli_input_line[i]);

                break;
            case CLI_HELP_ID:
                cli_help();
                break;
            case CLI_QUIT_ID:
                cli_exit_from_tlearn();
                break;
            case CLI_VERSION_ID:
                cli_display_info();
                break;
            case CLI_TRAIN_ID:
                report_condition("Training...", 0);
                learning= 1;
                verify= 0;
                probe= 0;
                if (cli_run_system() == NO_ERROR) {
                    report_condition("Training complete.", 0);
                    sprintf(loadfile, "%s.%ld.wts", root, sweep);
                    loadflag= 1;
                    sprintf(message, "Weights file:     \"%s\"\n", loadfile);
                    report_condition(message, 0);
                }
                else report_condition("Training halted.", 0);
                gRunning= 0;
                break;
            case CLI_VERIFY_ID:
                report_condition("Verifying...", 0);
                learning= 0;
                verify= 1;
                probe= 0;
                if (cli_run_system() == NO_ERROR)
                    report_condition("Verify complete.", 0);
                else report_condition("Verify halted.", 0);
                gRunning= 0;
                break;
            case CLI_PROBE_ID:
                report_condition("Probing...", 0);
                learning= 0;
                verify= 0;
                probe= 1;
                if (cli_run_system() == NO_ERROR)
                    report_condition("Probe complete.", 0);
                else report_condition("Probe halted.", 0);
                gRunning= 0;
                break;
            case CLI_SHELL_ID:
                strcpy(shell_line, NO_WORD);
                while(strcmp(strcpy(word1, get_next_word()), NO_WORD) != 0) {
                    strcat(shell_line, word1);
                    strcat(shell_line, " ");
                }
                system(shell_line);
                break;
            case CLI_LOAD_ID:
                break;
            case CLI_SAVE_ID:
                break;
            case CLI_LAST_ID:
                break;
            case CLI_NEXT_ID:
                break;
            case CLI_DEFAULT_ID:
                report_condition("Restoring default settings...", 1);
                cli_restore_default_settings();
                break;
            case CLI_ERROR_ID:
                break;
            case CLI_ARCHITECTURE_ID:
                break;
            case CLI_ACTIVATIONS_ID:
                break;
            case CLI_WEIGHTS_ID:
                break;
            case CLI_ALIAS_ID:
                strcpy(word1, get_next_word());
                strcpy(word2, get_next_word());
                if (strcmp(word1, NO_WORD) == 0) {
                    cli_help();
                }
                else if (strcmp(word2, NO_WORD) == 0) {
                    sprintf(message, "Alias \"%s\" to what command?", word1);
                    report_condition(message, 2);
                }
                else {
                    new_command= retrieve_command(word1);
                    if (new_command.action_id == CLI_UNKNOWN_ID) {
                        /* Command doesn't exist... */
                        add_command(word1, CLI_EMPTY_ID, word2);
                    }
                    else {
                        /* Command exists... */
                        change_command(new_command, word1, CLI_EMPTY_ID, 
                                       word2);
                    }
                }
                break;
            case CLI_STATUS_ID:
                show_status();
                break;
            case CLI_UNKNOWN_ID:
                break;
            case CLI_EMPTY_ID:
                break;
            case CLI_FILEROOT_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) strcpy(root, word1);
                else report_condition("Set file root to what?", 2);
                break;
            case CLI_WEIGHTS_FILE_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) {
                    strcpy(loadfile, word1);
                    sprintf(message, "Load weights from:        \"%s\"", loadfile);
                    report_condition(message, 0);
                    loadflag= 1;
                }
                else {
                    report_condition("Weights file turned off.", 0);
                    loadflag= 0;
                }
                break;
            case CLI_SWEEPS_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) nsweeps= atoi(word1);
                else report_condition("How many sweeps?", 2);
                break;
            case CLI_WEIGHTS_MOMENTUM_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) momentum= atof(word1);
                else report_condition("Set momentum to what?", 2);
                break;
            case CLI_LEARNING_RATE_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) rate= atof(word1);
                else report_condition("Set learning rate to what?", 2);
                break;
            case CLI_ERROR_TYPE_ID:
                ce= !ce;
                printf("Error Type:     ");
                if (ce)printf("cross-entropy\n");
                else printf("root-mean-square\n");
                break;
            case CLI_ERROR_REPORT_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) rms_report= atoi(word1);
                else report_condition("How many sweeps per error report?", 2);
                break;
            case CLI_ERROR_STOP_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) criterion= atof(word1);
                else report_condition("Stop when error falls below what?", 2);
                break;
            case CLI_WEIGHTS_UPDATE_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) umax= atoi(word1);
                else report_condition("How many sweeps per weights update?", 2);
                break;
            case CLI_WEIGHTS_SAVE_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) check= atoi(word1);
                else report_condition("How many sweeps per weights save?", 2);
                break;
            case CLI_BIAS_OFFSET_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) init_bias= atof(word1);
                else report_condition("Set bias offset to what?", 2);
                break;
                case CLI_BPTT_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) {
                    copies= atoi(word1);
                    if (copies < 2) {
                        bptt= 0;
                        sprintf(message, "BPTT:                 OFF");
                    }
                    else {
                        bptt= 1;
                        sprintf(message, "BPTT timesteps:       %i", copies);
                    }
                    report_condition(message, 0);
                }
                else report_condition("BPTT?", 2);
                break;
            case CLI_SEED_ID:
                strcpy(word1, get_next_word());
                if (strcmp(word1, NO_WORD) != 0) {
                    seed= atoi(word1);
                    if (seed <= 0) {
                        fixedseed= 0;
                        sprintf(message, "Seed:                 OFF");
                    }
                    else {
                        fixedseed= 1;
                        sprintf(message, "Seed:         %i", seed);
                    }
                    report_condition(message, 0);
                }
                else report_condition("Seed?", 2);
                break;
            case CLI_RESET_FILE_ID:
                rflag= !rflag;
                if (rflag)
                    sprintf(message, "Reset File:                       ON");
                else
                    sprintf(message, "Reset File:                       OFF");
                report_condition(message, 0);
                break;
            case CLI_RANDOM_ORDERING_ID:
                randomly= !randomly;
                if (randomly)
                    sprintf(message, "Random Ordering:          ON");
                else
                    sprintf(message, "Random Ordering:          OFF");
                report_condition(message, 0);
                break;
            case CLI_TEACHER_FORCING_ID:
                teacher= !teacher;
                if (teacher)
                    sprintf(message, "Teacher Forcing:          ON");
                else
                    sprintf(message, "Teacher Forcing:          OFF");
                report_condition(message, 0);
                break;
            default:
                report_condition("Internal error: Command unmapped to any action!", 2);
                break;
        }
    }
}



        /* Recursively prints a command and its aliased commands. */
static void print_command(Command command)
{
    if (command.action_id != CLI_UNKNOWN_ID) printf("%s", command.word);
    else if (command.action_id == CLI_UNKNOWN_ID) printf("<no mapping>");
    if (strcmp(command.alias, NO_ALIAS) != 0) {
        printf("        = ");
        print_command(retrieve_command(command.alias));
    }
    else {
        printf("\n");
    }
}


        /* Prints out all available commands and aliases. */
static void cli_help(void)
{
    int i;
    printf("List of available commands:\n\n");
    for(i= 1; i < total_commands; i++) 
        print_command(commands[i]);

    printf("\nCommands that are also parameter names take arguments, e.g.:");
    printf("\n file_root xor   <or>   seed 123   <or>   random_ordering ON");
    printf("\nCommands which xtlearn cannot understand are piped to the shell.\n");
    printf("To force command piping to the shell script, use \"shell [command]\".\n");
}


        /* Quits the program. */
static void cli_exit_from_tlearn(void)
{
    exit(0);
}


        /* Shows the current parameter settings. */
static void show_status(void)
{
    printf("Current status:\n\n");
    printf("File Root:\t\t%s\n", *root ? root : "<Not set.>");

    printf("Weights File:\t\t");
    if (loadflag) printf("%s\n", loadfile);
    else printf("<Don't load.>\n");

    printf("Simultation Sweeps:\t%ld\n", nsweeps);
    printf("Weights Momentum:\t%f\n", momentum);
    printf("Learning Rate:\t\t%f\n", rate);
    printf("Error Type:\t\t");
    if (ce == 0) printf("root-mean-square\n");
    else printf("cross-entropy\n");

    printf("Sweeps/Error Report:\t%ld\n", rms_report);
    printf("Error Stop Point:\t%f\n", criterion);
    printf("Sweeps/Weights Update:\t%ld\n", umax);
    printf("Sweeps/Weights Save:\t%ld\n", check);
    printf("Bias Offset:\t\t%f\n", init_bias);
    printf("BPTT Timesteps:\t\t");
    if (bptt) printf("%i\n", copies);
    else printf("<Not invoked.>\n");

    printf("Random Number Seed:\t");
    if (fixedseed) printf("%i\n", seed);
    else printf("<Not fixed.>\n");

    printf("Reset File:\t\t");
    if (rflag) printf("ON\n");
    else printf("OFF\n");

    printf("Random Input Ordering:\t");
    if (randomly) printf("ON\n");
    else printf("OFF\n");

    printf("Teacher Forcing:\t");
    if (teacher) printf("ON\n");
    else printf("OFF\n");
}



        /* Sets parameters to default values. */
static void cli_restore_default_settings(void)
{
    strcpy(root, NO_WORD);
    root[0]=0;
    strcpy(loadfile, NO_WORD);
    loadflag= 0;
    nsweeps= 1000;
    bptt= 0;
    copies= 2;
    momentum= 0.0;
    rate= 0.1;
    fixedseed= 0;
    seed= 0;
    rms_report= 0;
    criterion= 0.;
    umax= 0;
    check= 0;
    init_bias= 0.;

    randomly= 0;
    rflag= 0;
    teacher= 0;
}

        /* Shows version information. */
static void cli_display_info(void)
{
    char dummy[80];

    sprintf(dummy, "%s version %s,\nby %s\n@crl.ucsd.edu, %s.%s", 
        PROGRAM_NAME, PROGRAM_VERSION, PROGRAMMERS, PROGRAM_DATE,
#ifdef EXP_TABLE
	"\nRequires exp table: "EXP_TABLE);
#else
        "");
#endif
    report_condition(dummy, 0);
}


        /* CLI version of simulation loop. */
static int cli_run_system(void)
{
    int sweeps_per_update= 0;
    int update_time= 0;
    int total_ticks= 0;
    int i= 0;

    if (nsweeps < 1) return report_condition("You forgot to set the number of sweeps...", 2);
    sweeps_per_update= nsweeps / CLI_ERROR_POINTS;

    if (set_up_simulation()) return ERROR;

    fprintf(stdout, " 0%%    20%%     40%%     60%%     80%%     100%%\n");
    fprintf(stdout, "   .   .   .   .   .   .   .   .   .   .<\r");
    fflush(stdout);
    nsweeps += tsweeps;
    sweep= tsweeps;
    gAbort= 0;
    gRunning= 1;
    while (nsweeps > sweep && !gAbort) {
        if (update_time++>sweeps_per_update) {
            update_time= 0;
            total_ticks++;
            fprintf(stdout, "|");
            fflush(stdout);
        }
        if (TlearnLoop()) return ERROR;
        sweep++;
    }
    if (learning) {
        if (save_wts() == ERROR) return ERROR;
    }
    for(i=total_ticks;i < CLI_ERROR_POINTS;i++) {
        printf("|");
    }
    fprintf(stdout, "\n");
    return NO_ERROR;
}
