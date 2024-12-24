#ifndef GASMIC_AS_H
#define GASMIC_AS_H

/*#include <stdarg.h>*/
#include <stdio.h>
/*////#include <stdlib.h>*/
/*#include <string.h>
#include <ctype.h>*/
#include "fixedint.h"
/*#include <lang.h>
#include <smem.h>*/
#include "noreturn.h"

/*#include <console.h>
#include <ansistyle.h>*/

/* print if debug build *//*
#ifndef NDEBUG
*//* helper debug printf *//*
static INLINE int h_printdf(const char *fname, const int linenum, const char *fmt, ...)
{
	int count = 0;
	va_list args;
	va_start(args, fmt);

	count += printf("[%s:%d] >> ", fname, linenum);
	count += vprintf(fmt, args);

	va_end(args);
	return count;
}
*//*#define printdf(...) h_printdf(__FILE__, __LINE__, __VA_ARGS__)*//*

#else
#define printdf(fmt, ...)
#endif*/

/* This is to stop clang-tidy from complaining about signed enum types */
#ifdef __cplusplus
#define FLAG(f) (f)
#else
#define FLAG(f) ((unsigned int)(f))
#endif

enum endian {
	ARCH_ENDIAN_BIG = 1,
	ARCH_ENDIAN_LITTLE = 2,
	ARCH_ENDIAN_MIXED = 3
};

enum syntax {
	SYNTAX_UNKNOWN,
	SYNTAX_MOTOROLA,
	SYNTAX_INTEL,
	SYNTAX_ATT
};

enum arg_order {
	ARG_ORDER_NONE,
	ARG_ORDER_FROM_REG,
	ARG_ORDER_TO_REG,
	ARG_ORDER_INTERREG
};

#if 0
enum address_type {
	ADDRESS_TYPE_ABSOLUTE;
	ADDRESS_TYPE_RELATIVE;
};
struct address {
	enum address_type type;
	union {
		size_t absolute;
		struct {
			struct token *root_expr;
			size_t offset;
		};
	} value;
};
#endif

struct symbol {
	char *label;
	int64_t value;
	struct symbol *next;
};

struct symboltab{
	struct symbol *first;
	struct symbol *last;
	struct symbol *last_parent;
};

enum data_type {
	DATA_TYPE_NONE,
	DATA_TYPE_EXPRESSION,
	DATA_TYPE_BYTES
};
typedef struct data_entry {
	enum data_type type;
	size_t address;
	uint8_t bytec;
	union {
		uint8_t *bytes;
		struct token *rpn_expr;
	} contents;
	struct data_entry *next;
} Data;

typedef struct {
	Data *first;
	Data *last;
} DataTab;

typedef struct {
	char name[5];
	int width;
	unsigned int arcs;
} Register;

#define MAX_MNEMONIC_LEN (10)

enum address_mode {
	ADDR_MODE_INVALID = 0,
	ADDR_MODE_INHERENT = (1u << 0u),
	ADDR_MODE_IMMEDIATE = (1u << 1u),
	ADDR_MODE_DIRECT = (1u << 2u),
	ADDR_MODE_INDIRECT = (1u << 3u),
	ADDR_MODE_EXTENDED = (1u << 4u),
	ADDR_MODE_INDEXED = (1u << 5u),
	ADDR_MODE_INTERREGISTER = (1u << 7u),
	ADDR_MODE_INDEXED_INDIRECT = (FLAG(ADDR_MODE_INDEXED) | FLAG(ADDR_MODE_INDIRECT))
};

#define ADDRESS_MODE_COUNT (8)
typedef struct {
	char mnemonic[MAX_MNEMONIC_LEN];
	uint8_t architectures;
	uint8_t arg_order;
	struct instruction_register {
		const Register *reg;
		struct {
			enum address_mode mode;
			uint8_t opcode_size;	/* in bytes */
			uint64_t opcode;
		} addressing_modes[10];
	} opcodes[ADDRESS_MODE_COUNT];
} Instruction;

enum arg_type {
	ARG_TYPE_UNPROCESSED,
	ARG_TYPE_STRING,
	ARG_TYPE_EXPRESSION,
	ARG_TYPE_REGISTER,
	ARG_TYPE_INDEX,
	ARG_TYPE_INDEX_REGISTER,
	ARG_TYPE_INDEX_CONSTANT
};

typedef struct {
	enum arg_type type;
	/*enum arg_state state;*/
	/*enum address_mode addr_mode;*/
	union {
		char *str;
		struct token *rpn_expr;
		const Register *reg;
		struct {
			const Register *base;
			union {
				struct token *expression;
				const Register *reg;
			} offset;
			int8_t pre_inc;
			int8_t post_inc;
		} indexed;
	} val;
} LineArg;

enum line_state {
	LINE_STATE_CLEAR		= 0,
	LINE_STATE_LABEL		= (1u << 0u),
	LINE_STATE_MNEMONIC		= (1u << 1u),
	LINE_STATE_SINGLE_QUOTE = (1u << 2u),
	LINE_STATE_DOUBLE_QUOTE = (1u << 3u),
	LINE_STATE_BRACKET		= (1u << 4u)
};

enum address_post_op {
	POST_OP_NONE,
	POST_OP_INC_SINGLE,
	POST_OP_INC_DOUBLE,
	POST_OP_DEC_SINGLE,
	POST_OP_DEC_DOUBLE
};

struct line {
	char *label;
	char *mnemonic;
	LineArg *argv;
	size_t argc;
	uint8_t arg_buf_size;
	enum line_state line_state;
	enum address_mode address_mode;
	enum address_post_op addr_mode_post_op;
};

typedef struct {
	char *name;
	int value;
	uint8_t byte_size;  /* bits per byte */
	uint8_t bytes_per_address;
	enum endian endianness;
	enum syntax default_syntax;
	const Register *registers;
	const Instruction **instructions;
	void (*process_line)(struct line *line, const struct instruction_register *instr_reg, Data *data);
} Architecture;

struct configuration {
	char				*out_fname;
	char				**in_fnames;
	char				*export_fname;
	size_t				in_fnamec;
	size_t 				in_fname_size;
	uint8_t				syntax;
	const Architecture 	*arch;
};

struct context {
	FILE *fptr;
	char *fname;
	struct context *parent;
	size_t line_num;
};

extern struct symboltab symtab;
extern DataTab *datatab;

extern size_t address;
extern size_t address_mask;
extern struct configuration g_config;
extern struct context *g_context;

void init_address_mask();
void assemble(struct context *cntxt);
NORETURN void fail(const char *fmt, ...);

const Architecture *find_arch(const char *arch_name);
const Register *find_reg(const char *name);

int init_data_table();
Data *init_data(Data *data);
void add_label(struct line *line);
void add_data(Data *data);

void prepare_line(struct line *line);

#endif /* GASMIC_AS_H */
