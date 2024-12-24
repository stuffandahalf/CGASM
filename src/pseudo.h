#ifndef GASMIC_PSEUDO_H
#define GASMIC_PSEUDO_H

/*#include "as.h"*/

struct pseudo_instruction {
	/*char instruction[10];*/
	char *instruction;
	void (*process)(struct line *line);
	short int args;
};

struct pseudo_instruction *get_pseudo_op(struct line *line);
void parse_pseudo_op(struct line *line);

#endif /* GASMIC_PSEUDO_H */
