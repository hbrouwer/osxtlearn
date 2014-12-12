/*

     error.c  General error handling functions.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
#include "xtlearnIface.h"
#include "error.h"

#define MAX_ERRORMSG_LEN 1024

extern int interactive, batch_mode;

static FILE *FilePtr;
static char FileName[128]; // XXX: instead of 40
static char FileType[20];
static char ErrorMsg[MAX_ERRORMSG_LEN];

static int SilencingErrors;

static long display_bad_line(void);



        /* Call parse_err_set before parsing a file to
           simplify error handling */

void SilenceErrors(short silence)
{
    SilencingErrors= silence;
}



void parse_err_set(FILE *filePtr, char *fileName, char *fileType)
{
    FilePtr= filePtr;
    strcpy(FileName, fileName);
    strcpy(FileType, fileType);
}


        /* parse_err displays the line that caused the parse error
           and a message passed to it */

int parse_err(char *err_text)
{
    int line_no;

    if (SilencingErrors) return -1;

    line_no= display_bad_line();
    sprintf(ErrorMsg, "%s file problem:  %s  Check on or near line %d.", FileType, err_text, line_no);
    report_condition(ErrorMsg, 2);
    return ERROR;
}


        /* Generic error reporting function */

int report_condition(char *message, int severity)
{
    if (SilencingErrors && severity < 3) return -1;

#ifdef XTLEARN

    if (!(interactive || batch_mode) && severity < 3) {
        XReportCondition(message, severity);
        return ERROR;
    }

#endif

    switch(severity) {
        case 0:  fprintf(stderr,"%s\n", message); break;
        case 1:  fprintf(stderr,"Warning: %s\n", message); break;
        case 2:  fprintf(stderr,"Error: %s\n", message); break;
        default: fprintf(stderr,"*** Fatal Error: %s\n", message);
                 fprintf(stderr, "\nPlease report this bug, along with any message\n");
                 fprintf(stderr, "which is displayed above, to:\n%s", BUG_REPORT);
                 exit(1);
    }

    return ERROR;
}


#define PARSE_LINES 4

static long display_bad_line(void)
{
    int no_lines, len, i;
    char *tmpptr= ErrorMsg;
    long linestarts[PARSE_LINES], lineNumber= 0;
    long file_offset= ftell(FilePtr);

    for (i= 0; i < PARSE_LINES; i++) linestarts[i]= 0L;

    fseek(FilePtr, 0L, 0);

    while (1) {
        for (i= 0; i < PARSE_LINES -1; i++) linestarts[i]= linestarts[i+1];
        linestarts[PARSE_LINES -1]= ftell(FilePtr);
        if (linestarts[PARSE_LINES -1] > file_offset) break;
        fgets(ErrorMsg, MAX_ERRORMSG_LEN, FilePtr);
        lineNumber++;
    }
    
    no_lines= (lineNumber >= PARSE_LINES) ? PARSE_LINES -1 : lineNumber;
    sprintf(ErrorMsg, "Last %d lines of %s parsed:\n\n", no_lines, FileName);

    fseek(FilePtr, linestarts[0], 0);
    for (i= 0; i < no_lines; i++) {
        tmpptr= strchr(ErrorMsg, 0);
        fgets(tmpptr, MAX_ERRORMSG_LEN/PARSE_LINES, FilePtr);
    }

    len= file_offset -linestarts[no_lines -1] -1;
    for (i= 0; i < len; i++) {
        if (tmpptr[i] == '\t') strcat(ErrorMsg, "\t");
        else strcat(ErrorMsg, " ");
    }

    sprintf(strchr(ErrorMsg, 0), "^ Error noticed here - line %ld.\n", lineNumber);

    report_condition(ErrorMsg, 0);

    return lineNumber;
}


short memerr(char *array_name)
{
    sprintf(ErrorMsg, "Not enough memory to work with this network.  FYI: %s allocation failed.", array_name);
    return report_condition(ErrorMsg, 2);
}
