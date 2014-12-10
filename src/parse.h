#ifndef PARSE_H
#define PARSE_H

/* parse.h */

int get_nodes(void);
int get_outputs(void);
int get_connections(void);
int get_special(void);
int get_nums(char *str, int nv, int offset, int *vec);

#endif
