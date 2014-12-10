#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>

#define ERROR   -1
#define NO_ERROR 0

	/* Bug reporting: */
#define BUG_REPORT "\nJeff Elman\nCenter for Research in Language\nUCSD\nelman@crl.ucsd.edu\n\nand your system administrator.\n"

void parse_err_set(FILE *file_ptr, char *filename, char *filetype);
void SilenceErrors(short silence);
int parse_err(char *err_text);
short memerr(char *array_name);
int report_condition(char *message, int severity);

#endif
