#ifndef _AS_H
#define _AS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <smem.h>

//#define DEBUG

#define streq(__s1, __s2) !strcmp((const char *)__s1, (const char *)__s2)

#ifndef NDEBUG
#define printdf(fmt, ...) printf("[%s:%d] >> " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define printdf(fmt, ...)
#endif

#define ARCH_BIG_ENDIAN (1)
#define ARCH_LITTLE_ENDIAN (2)

#define SYNTAX_UNKNOWN  (0)
#define SYNTAX_MOTOROLA (1)
#define SYNTAX_INTEL    (2)
#define SYNTAX_ATT      (3)

#define ARG_ORDER_NONE      (0)
#define ARG_ORDER_FROM_REG  (1)
#define ARG_ORDER_TO_REG    (2)
#define ARG_ORDER_INTERREG  (3)

typedef struct symtab_entry {
    char *label;
    size_t value;
    struct symtab_entry *next;
} Symbol;

typedef struct {
    Symbol *first;
    Symbol *last;
    Symbol *last_parent;
} SymTab;

#define DATA_TYPE_LABEL 1
#define DATA_TYPE_BYTES 2
typedef struct data_entry {
    uint8_t type;
    uint64_t address;
    uint8_t bytec;
    union {
        char *symbol;
        uint8_t *bytes;
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
    int arcs;
} Register;

#define MAXMNEMONICSIZE (10)
#define MAX_REGS (20)
typedef struct {
    char mnemonic[MAXMNEMONICSIZE];
    uint8_t architectures;
    uint16_t base_opcode;
    uint8_t arg_order;
    uint8_t address_modes;
    Register *registers[MAX_REGS];
} Instruction;

#define ARG_TYPE_NONE 0
#define ARG_TYPE_REG  1
#define ARG_TYPE_SYM  2
#define ARG_TYPE_STR  3
#define ARG_STATE_BRACKET 1

typedef struct {
    uint8_t type;
    //uint8_t addr_mode;
    union {
        char *str;
        Register *reg;
        Symbol *sym;
    } val;
} LineArg;

#define LINE_STATE_LABEL (1)
#define LINE_STATE_MNEMONIC (2)
#define LINE_STATE_QUOTE (4)
#define LINE_STATE_BRACKET (8)
typedef struct {
    char *label;
    char *mnemonic;
    LineArg *argv;
    //char **argv;
    size_t argc;
    uint8_t arg_buf_size;
    uint8_t line_state;
} Line;

typedef void (*parse_line_t)(Line *l);

typedef struct {
    char name[10];
    void (*parse_instruction)(Line *l);
    int value;
    uint8_t byte_size;  // bits per byte
    uint8_t bytes_per_address;
    uint8_t endianness;
    uint8_t default_syntax;
    parse_line_t parse_function;
    //Register *registers;
    //Instruction *instructions;
} Architecture;

struct pseudo_instruction {
    char instruction[10];
    void (*process)(Line *line);
    size_t args;
};

typedef struct {
    char *out_fname;
    char **in_fnames;
    size_t in_fnamec;
    uint8_t syntax;
    Architecture *arch;
} Config;

extern size_t line_num;
extern size_t address;
extern size_t address_mask;
extern SymTab *symtab;
extern DataTab *datatab;
extern Config configuration;

#define fail(msg, ...) die("\033[0;31mERROR \033[0m%ld: " msg, line_num,  ##__VA_ARGS__)

void assemble(FILE *in, Line *l);
Architecture *str_to_arch(const char arch_name[]);
struct pseudo_instruction *get_pseudo_op(Line *line);
void parse_pseudo_op(Line *line);
void add_data(Data *data);

#endif
