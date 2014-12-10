#ifndef TLEARN_H
#define TLEARN_H

	/* Version: */
#define PROGRAM_NAME "tlearn for the X Window System"
#define PROGRAM_VERSION "1.00"
#define	PROGRAM_DATE "May 1994"
#define PROGRAMMERS "John Kendall, David Spitz, Jeff Elman, Mark Dolson, etc."


int init_config(void);
int set_up_simulation(void);
int TlearnLoop(void);
int XTlearnLoop(void);
long GetFileModTime(char *fname);

#endif
