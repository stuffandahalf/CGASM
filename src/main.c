#include <stdio.h>
/*#ifdef _WIN32
#include <getopt.h>
#else
#include <unistd.h>
#endif*/
#include "getopt.h"
#include "console.h"
#include "as.h"
#include "targets.h"
#include "formats.h"
#include "pseudo.h"
#include "arithmetic.h"

/*
 * For each input file
 *   open new context
 *   parse line into opcode + args
 *   evaluate args
 *   translate opcode + args -> binary
 *   add binary to context
 *   generate output file in specified format
 * if link
 *   combine all processed files into output format
 */

#define LINEBUFFERSIZE (256)
char buffer[LINEBUFFERSIZE];

static int configure(int argc, char *const argv[]);
/*static void trim_str(char str[]);*/
static void parse_line(struct line *l, char *buffer);
static void evaluate_args(struct line *l);
static void resolve_mnemonic_type(struct line *l);
static void set_syntax_parser();
static void prepare_line_motorola(struct line *l);
static void prepare_line_att(struct line *l);
static void prepare_line_intel(struct line *l);

struct configuration g_config;
struct context *g_context;

/*FILE *in;*/
FILE *out;
size_t address = 0;
size_t address_mask;	/* bits to mask the address to;*/
void (*parse_instruction)(struct line *l);
/*SymTab *undefined_symtab;*/

#define TARGET(t) &ARCH_ ## t,
Architecture **architectures[] = { TARGETS NULL };  /* NULL terminated array of targets */
#undef TARGET

int
main(int argc, char *const argv[])
{
#if 0
	//struct token *rpn_expr = parse_expression("1 *(2 +3)");
	struct token *rpn_expr = parse_expression("(_start * 3) + tmp");
	//struct token *rpn_expr = parse_expression("(_start * 3 + tmp");
	print_token_list(rpn_expr);
	printf("%d\n", arithmetic_status_code);
	free_token_chain(rpn_expr);
	return 0;
#endif

	size_t i;
	struct context init_cntxt;
	struct symbol *sym, *tmp_sym;
	//Data *data, *tmp_data;

#ifdef _WIN32
	if (!init_console()) {
		printef("WARNING\tFailed to initialize terminal, colour not supported\n");
	}
#endif
	init_targets();

	/*FILE *in;*/
	/*out = stdout;*/

	if (!configure(argc, argv)) {
		goto early_exit;
	}

	/*out = fopen(out_fname, "w+");*/
	/*free(g_config.out_fname);*/
	

	/*struct context init_cntxt;*/
	g_context = &init_cntxt;
	init_cntxt.line_num = 1;
	init_cntxt.parent = NULL;

	init_address_mask();
	set_syntax_parser();

	init_data_table();

	if (g_config.in_fnamec < 0) {
		die("Invalid number of command line arguments.\n");
	} else if (g_config.in_fnamec == 0) {
		if ((init_cntxt.fname = saquire(str_clone("stdin"))) == NULL) {
			die("Failed to duplicate file name \"stdin\"\n");
		}
		init_cntxt.fptr = stdin;
		assemble(&init_cntxt);
		sfree(init_cntxt.fname);
	} else {
		for (i = 0; i < g_config.in_fnamec; i++) {
			if ((init_cntxt.fname = saquire(str_clone(g_config.in_fnames[i]))) == NULL) {
				die("Failed to duplicate file name \"%s\"\n", g_config.in_fnames[i]);
			}
			if ((init_cntxt.fptr = fopen(init_cntxt.fname, "r")) == NULL) {
				die("Failed to open input file %s\n", g_config.in_fnames[i]);
			}
			init_cntxt.line_num = 1;
			assemble(&init_cntxt);
			fclose(init_cntxt.fptr);
			sfree(init_cntxt.fname);
		}
	}
	sfree(g_config.in_fnames);
	g_config.in_fnames = NULL;
	g_config.in_fnamec = 0;
	g_config.in_fname_size = 0;

	/* TODO: Resolve references here */

#if 0
	if (g_config.export_fname != NULL) {
		FILE *export_file;

		export_file = fopen(g_config.export_fname, "w+");
		if (export_file == NULL) {
			die("Failed to open export file \"%s\"\n", g_config.export_fname);
		}
		sym = symtab.first;
		while (sym != NULL) {
			fprintf(export_file, "%s: EQU %" PRId64 "\n", sym->label, sym->value);
			sym = sym->next;
		}
		fclose(export_file);
	}
#endif

	printdf(("SYMBOLS\n"));
	sym = symtab.first;
	while (sym != NULL) {
		printdf(("%s = %" PRId64 "\n", sym->label, sym->value));

		sfree(sym->label);
		sym->label = NULL;
		tmp_sym = sym;
		sym = sym->next;
		sfree(tmp_sym);
	}
	/*sfree(symtab->first->label)*/;
	/*sym = NULL;*/

#if 0
	printdf(("DATATAB\n"));
	data = datatab->first;
	while (data != NULL) {
		printdf(("data address: " SZXFMT ",  ", data->address));
		switch (data->type) {
		case DATA_TYPE_EXPRESSION:
			/*printdf("%" PRIu8 " bytes label \"%s\"\n", data->bytec, data->contents.symbol);*/
			/*sfree(data->contents.symbol);*/
#ifndef NDEBUG
			printf("RPN expression: ");
			print_token_list(data->contents.rpn_expr);
#endif
			free_token_chain(data->contents.rpn_expr);
			break;
		case DATA_TYPE_BYTES:
#ifndef NDEBUG
			printf("%" PRIu8 " bytes: ", data->bytec);
			for (i = 0; i < data->bytec; i++) {
				if (i) {
					printf(", ");
				}
				printf("%" PRIX8, data->contents.bytes[i]);
			}
			printf("\n");
#endif
			sfree(data->contents.bytes);
			break;
		case DATA_TYPE_NONE:
#ifndef NDEBUG
			printf("Empty data.\n");
#endif
			break;
		default:
#ifndef NDEBUG
			printf("Garbage data.\n");
#endif
			break;
		}
		tmp_data = data;
		data = data->next;
		sfree(tmp_data);
	}
	sfree(datatab);
	datatab = NULL;
	data = NULL;
	/*close(out);*/
#endif

	g_context = NULL;

early_exit:
	destroy_targets();

#ifdef _WIN32
#ifndef NDEBUG
	printef("Press any key to exit.\n");
	getc(stdin);
#endif
	restore_console();
#endif

	return 0;
}

void
init_address_mask()
{
	int i;
	address_mask = 0;
	for (i = 0; i < g_config.arch->bytes_per_address * g_config.arch->byte_size; i++) {
		if (i) {
			address_mask <<= 1u;
		}
		address_mask |= 1u;
	}
	printdf(("Address mask: " SZXFMT "\n", address_mask));
}

void
assemble(struct context *cntxt)
{
	size_t i;
	struct line l;

	while (fgets(buffer, LINEBUFFERSIZE, g_context->fptr) != NULL) {
		if (buffer[0] != '\0' && buffer[0] != '\n') {
			l.line_state = LINE_STATE_CLEAR;
			l.address_mode = ADDR_MODE_INVALID;
			l.addr_mode_post_op = POST_OP_NONE;
			l.argc = 0;

			parse_line(&l, buffer);

#ifndef NDEBUG
			/*printf("%s\t%s", l.label, l.mnemonic);*/
			if (l.line_state & FLAG(LINE_STATE_LABEL)) {
				printf("%s", l.label);
			}
			if (l.line_state & FLAG(LINE_STATE_MNEMONIC)) {
				if (l.line_state & FLAG(LINE_STATE_LABEL)) {
					puts("\t");
				}
				printf("%s", l.mnemonic);
			}
			for (i = 0; i < l.argc; i++) {
				printf("\t%s", l.argv[i].val.str);
			}
			puts("");
#endif
			if (l.line_state & FLAG(LINE_STATE_LABEL)) {	  /* If current line has a label */
				add_label(&l);
			}
			if (l.line_state & FLAG(LINE_STATE_MNEMONIC)) {   /* If current line has a mnemonic */
				str_to_upper(l.mnemonic);
				evaluate_args(&l);
				resolve_mnemonic_type(&l);
			}
		}
		g_context->line_num++;
	}
	if (ferror(g_context->fptr)) {
		fail("Too many characters entered.\n");
	}
}

static int
configure(int argc, char *const argv[])
{
	static const char *const help_str = "Usage: %s [-m arch] [-o outfile] [-f outformat] [-e symfile]\n";
	int c;

	g_config.arch = *architectures[0];
	g_config.syntax = g_config.arch->default_syntax;
	g_config.out_fname = "a.out";
	g_config.in_fname_size = 1;
	g_config.in_fnamec = 0;
	g_config.in_fnames = salloc(sizeof(char *) * g_config.in_fname_size);
	g_config.export_fname = NULL;

	while ((c = getopt(argc, argv, "-hm:o:f:e:")) != -1) {
		switch (c) {
		case 1:
			printdf(("Found file argument %s\n", optarg));
			if (g_config.in_fnamec == g_config.in_fname_size) {
				g_config.in_fname_size += 2;
				g_config.in_fnames = srealloc(g_config.in_fnames, g_config.in_fname_size);
			}
			g_config.in_fnames[g_config.in_fnamec++] = optarg;
			break;
		case 'm':	/* architecture */
			g_config.arch = find_arch(optarg);
			if (g_config.arch == NULL) {
				/*free(g_config.out_fname);*/
				die("Unsupported architecture: %s\n", optarg);
			}
			break;
		case 'o':	/* output file */
			/*free(g_config.out_fname);*/
			/*if ((g_config.out_fname = strdup(optarg)) == NULL) {
				die("Failed to allocate new output file name");
			}*/
			g_config.out_fname = optarg;
			break;
		case 'f':	/* output file format */
			break;
		case 'e':   /* export symbol table */
			g_config.export_fname = optarg;
			break;
		case 'h':
		case '?':
			printf(help_str, argv[0]);
			return 0;
		}
	}

	/*printdf(("argcount = %d\n", argc - optind));*/

	/*g_config.in_fnames = argv + sizeof(char) * optind;*/
	/*g_config.in_fnamec = argc - optind;*/

	return 1;
}

static void
parse_line(struct line *l, char *buffer)
{
	register char *c;
	struct line_arg *la = NULL;
	enum arg_type arg_type = ARG_TYPE_UNPROCESSED;
	for (c = buffer; *c != '\0'; c++) {
		switch (*c) {
		case '"':
			if (l->line_state & FLAG(LINE_STATE_SINGLE_QUOTE)) {
				break;
			} else if (!(l->line_state & FLAG(LINE_STATE_DOUBLE_QUOTE)) && c != buffer) {
				fail("Quotes must occur at the beginning of a field.\n");
			}
			l->line_state ^= FLAG(LINE_STATE_DOUBLE_QUOTE);
			if (l->line_state & FLAG(LINE_STATE_DOUBLE_QUOTE)) {
				arg_type = ARG_TYPE_STRING;
				buffer++;
			} else {
				*c = '\0';
			}
			break;
		case '\'':
			if (l->line_state & FLAG(LINE_STATE_DOUBLE_QUOTE)) {
				break;
			} else if (!(l->line_state & FLAG(LINE_STATE_SINGLE_QUOTE)) && c != buffer) {
				fail("Quotes must occur at the beginning of a field.\n");
			}
			l->line_state ^= FLAG(LINE_STATE_SINGLE_QUOTE);
			if (l->line_state & FLAG(LINE_STATE_SINGLE_QUOTE)) {
				arg_type = ARG_TYPE_STRING;
				buffer++;
			} else {
				*c = '\0';
			}
			break;
		case ']':
			if (l->line_state & FLAG(LINE_STATE_BOUNDED)) {
				break;
			}
			if (!(l->line_state & FLAG(LINE_STATE_BRACKET))) {
				fail("']' requires '[' first.");
			}
		case '[':
			if (l->line_state & FLAG(LINE_STATE_BOUNDED)) {
				break;
			}
			l->line_state ^= FLAG(LINE_STATE_BRACKET);
			break;

		case '\t':
		case ' ':
			if (l->line_state & FLAG(LINE_STATE_BOUNDED)) {
				break;
			}
			if (c == buffer) {
				buffer++;
			}
			/*else if (l->line_state & LINE_STATE_MNEMONIC) {
				fail("Mnemonic already set.\n");
			}*/
			else if (!(l->line_state & FLAG(LINE_STATE_MNEMONIC))) {
				*c = '\0';
				l->mnemonic = buffer;
				l->line_state |= FLAG(LINE_STATE_MNEMONIC);
				buffer = c;
				buffer++;
			}
			break;
		case ',':
			/*if (c == buffer) {
				break;
			}*/
			if (l->line_state & FLAG(LINE_STATE_BOUNDED)) {
				break;
			}
			if (!(l->line_state & FLAG(LINE_STATE_MNEMONIC))) {
				fail("No mnemonic preceding argument.\n");
			}
			if (l->argc == LINE_ARG_MAX) {
				fail("Too many arguments provided. (max %d)\n", LINE_ARG_MAX);
			}
			la = &(l->argv[l->argc++]);
			la->type = arg_type;
			//la->state = ARG_STATE_CLEAR;
			//la->addr_mode = ADDR_MODE_INVALID;
			la->val.str = buffer;
			*c = '\0';
			buffer = c;
			buffer++;
			arg_type = ARG_TYPE_UNPROCESSED;
			break;

		case '\n':
			if (l->line_state & (FLAG(LINE_STATE_SINGLE_QUOTE) | FLAG(LINE_STATE_DOUBLE_QUOTE))) {
				fail("Unterminated string constant.\n");
			}
			*c = '\0';
			if (!(l->line_state & FLAG(LINE_STATE_MNEMONIC))) {
				l->mnemonic = buffer;
				buffer = c;
				buffer++;
				l->line_state |= FLAG(LINE_STATE_MNEMONIC);
			} else {
				if (l->argc == LINE_ARG_MAX) {
					fail("Too many arguments provided. (max %d)\n", LINE_ARG_MAX);
				}
				la = &(l->argv[l->argc++]);
				la->type = arg_type;
				la->val.str = buffer;
				arg_type = ARG_TYPE_UNPROCESSED;
				/* *c = '\0';*/
			}
			buffer = c;
			buffer++;
			break;
		case ':':
			if (l->line_state & FLAG(LINE_STATE_BOUNDED)) {
				break;
			} else if (l->line_state & FLAG(LINE_STATE_LABEL)) {
				fail("Invalid label.\n");
			} else if (l->line_state & FLAG(LINE_STATE_MNEMONIC)) {
				fail("Label must occur at the beginning of a line.\n");
			} else if (arg_type == ARG_TYPE_STRING) {
				fail("Label cannot be a string literal.\n");
			}
			l->label = buffer;
			*c = '\0';
			buffer = c;
			buffer++;

			/*printdf("parsed literal label = %s\n", l->label);*/
			l->line_state |= FLAG(LINE_STATE_LABEL);
			break;
		case ';':
			return;
		}
		/*buffer++;	 // Why doesnt this work?*/
	}
	if (l->line_state & (FLAG(LINE_STATE_SINGLE_QUOTE) | FLAG(LINE_STATE_DOUBLE_QUOTE))) {
		fail("Unmatched quote.\n");
	}
	if (l->line_state & FLAG(LINE_STATE_BRACKET)) {
		fail("Unmatched bracket.\n");
	}
}

static void
evaluate_args(struct line *l)
{
	int i;
	for (i = 0; i < l->argc; i++) {
		printef("%s\t", l->argv[i].val.str);
	}
}

static void
resolve_mnemonic_type(struct line *line)
{
	struct pseudo_instruction *pseudo_op;

	if (line->mnemonic[0] == '\0') {
		return;
	} else if (line->mnemonic[0] == '.') {
		//prepare_line(line);
		parse_pseudo_op(line);
	} else if ((pseudo_op = get_pseudo_op(line)) != NULL) {
		//prepare_line(line);
		pseudo_op->process(line);
	} else {
		/*g_config.arch->parse_instruction(line);*/
		parse_instruction(line);
	}
}

static INLINE const struct instruction_register *
instruction_supports_reg(const Instruction *instruction, const Register *reg)
{
	const struct instruction_register *ir;
	for (ir = instruction->opcodes; ir->reg != NULL; ir++) {
		if (streq(reg->name, ir->reg->name)) {
			return ir;
		}
	}
	return NULL;
}

static void
set_syntax_parser()
{
	switch (g_config.syntax) {
	case SYNTAX_MOTOROLA:
		parse_instruction = &prepare_line_motorola;
		break;
	case SYNTAX_ATT:
		fail("AT&T instruction syntax is not yet implemented.\n");
		parse_instruction = &prepare_line_att;
		break;
	case SYNTAX_INTEL:
		fail("Intel instruction syntax is not yet implemented.\n");
		parse_instruction = &prepare_line_intel;
		break;
	case SYNTAX_UNKNOWN:
		fail("Unrecognized syntax.\n");
	}
}

static void
prepare_line_motorola(struct line *l)
{
	/*const char *instr_mnem = NULL;
	const char *line_mnemonic = NULL;
	const char *c;
	const Register *reg = NULL;
	const struct instruction_register *instruction_reg = NULL;
	struct LineArg *argcpy = NULL;  // store a copy of the arguments in case processing fails
	//size_t j;*/

	/*char *unified_arg_str;*/

	size_t arg_len = 0;
	size_t i = 0;
	char *c;
	char *tmp_c;
	char *unified_arg_str;
	char *buffer_ptr;
	struct line_arg *la;
	char *arg_dup;
	char *reg_name;
	const Register *reg;
	Data *data;
	struct {
		enum arg_type type;
		union {
			const Register *reg;
			struct token *expr;
			void *ptr;
		} val;
		int8_t pre_inc;
		int8_t post_inc;
	} arg_part;
	bool end;

	if (l->argc == 0) {
		l->address_mode = ADDR_MODE_INHERENT;
		goto proceed;
	}

	for (i = 0; i < l->argc; i++) {
		if (l->argv[i].type != ARG_TYPE_UNPROCESSED) {
			fail("Instructions should not have complex arguments.\n");
		}
		arg_len += strlen(l->argv[i].val.str);
	}
	arg_len += l->argc - 1; /* space for commas */
	arg_len++;

	unified_arg_str = salloc(sizeof(char) * arg_len);

	/*char *c = unified_arg_str;*/
	buffer_ptr = unified_arg_str;
	for (i = 0; i < l->argc; i++) {
		if (i) {
			*buffer_ptr++ = ',';
		}
		for (c = l->argv[i].val.str; *c != '\0'; c++) {
			*buffer_ptr++ = *c;
		}
	}
	*buffer_ptr = '\0';

	l->argc = 1;
	la = &l->argv[0];
	la->val.str = unified_arg_str;
	la->type = ARG_TYPE_UNPROCESSED;

		/* first try to figure out what kind of forced argument this is*/
	c = la->val.str;
	/*la->val.str = NULL;*/
	switch (*c) {
	case '#':
		l->address_mode |= FLAG(ADDR_MODE_IMMEDIATE);
		c++;
		break;
	case '>':
		l->address_mode |= FLAG(ADDR_MODE_EXTENDED);
		c++;
		break;
	case '<':
		l->address_mode |= FLAG(ADDR_MODE_DIRECT);
		c++;
		break;
	case '[':
		l->address_mode |= FLAG(ADDR_MODE_INDIRECT);
		c++;
		for (tmp_c = c; *tmp_c != '\0' && *tmp_c != ']'; tmp_c++);
		if (*tmp_c == '\0') {
			fail("Unterminated bracket for indirect addressing.\n");
		}
		*tmp_c = '\0';
		tmp_c++;
		while(*tmp_c == ' ' || *tmp_c == '\t') {
			tmp_c++;
		}
		if (*tmp_c != ',' && *tmp_c != '\0') {
			fail("Unexpected characters after indirect argument.\n");
		}
		break;
	default:
		break;
	}

	la->val.str = c;
	buffer_ptr = c;
	arg_part.type = ARG_TYPE_UNPROCESSED;
	arg_part.val.ptr = NULL;

	end = false;
	while (!end) {
		switch (*c) {
		case ' ':
		case '\t':
			if (buffer_ptr == c) {
				buffer_ptr++;
			}
			break;
		case ',':
		case '\0':
			if (*c == '\0') {
				end = true;
			} else {
				*c = '\0';
			}

			/* This doesnt quite work */
			while (*buffer_ptr == '-' || *buffer_ptr == '+') {
				if (*buffer_ptr == '-') {
					arg_part.pre_inc--;
				} else if (*buffer_ptr == '+') {
					arg_part.pre_inc++;
				}
				buffer_ptr++;
			}

			/*for (tmp_c = c, tmp_c--; *tmp_c == '-' || *tmp_c == '+'; tmp_c--) {
				if (*buffer_ptr == '-') {
					arg_part.pre_inc--;
				} else if (*buffer_ptr == '+') {
					arg_part.pre_inc++;
				}
				*tmp_c = '\0';
			}*/

			arg_dup = str_clone(buffer_ptr);
			if (arg_dup == NULL) {
				die("Failed to allocate new buffer for argument piece.\n");
			}
			arg_dup = str_trim(arg_dup);

			reg_name = str_clone(arg_dup);
			if (reg_name == NULL) {
				die("Failed to allocate new buffer for register name.\n");
			}
			reg_name = str_to_upper(reg_name);

			if (*arg_dup == '\0') {
				arg_part.type = ARG_TYPE_EXPRESSION;
				arg_part.val.expr = parse_expression("0");
			} else if ((reg = find_reg(reg_name)) != NULL) {
				arg_part.type = ARG_TYPE_REGISTER;
				arg_part.val.reg = reg;
			} else {
				arg_part.type = ARG_TYPE_EXPRESSION;
				arg_part.val.expr = parse_expression(arg_dup);
			}

			free(reg_name);
			free(arg_dup);
			if (!end) { /* not last argument */
				/* not last argument, check if indexed, handle appropriately */
				l->address_mode |= FLAG(ADDR_MODE_INDEXED);   /* more than one argument means indexed */
				switch (arg_part.type) {
				case ARG_TYPE_EXPRESSION:
					la->type = ARG_TYPE_INDEX_CONSTANT;
					la->val.indexed.offset.expression = arg_part.val.expr;
					break;
				case ARG_TYPE_REGISTER:
					la->type = ARG_TYPE_INDEX_REGISTER;
					la->val.indexed.offset.reg = arg_part.val.reg;
					break;
				default:
					fail("Invalid type for index offset.\n");
					break;
				}
			} else {
				/* last argument, check if arg type is indexed */
				if (l->address_mode & FLAG(ADDR_MODE_INDEXED)) {
					if (arg_part.type != ARG_TYPE_REGISTER) {
						fail("Indexed addressing base must be a register.\n");
					}
					la->val.indexed.base = arg_part.val.reg;
				} else {
					switch (l->address_mode) {
					case ADDR_MODE_IMMEDIATE:
					case ADDR_MODE_EXTENDED:
					case ADDR_MODE_DIRECT:
						if (arg_part.type != ARG_TYPE_EXPRESSION) {
							fail("Immediate argument must be an expression.\n");
						}
						la->type = ARG_TYPE_EXPRESSION;
						la->val.rpn_expr = arg_part.val.expr;
						break;
					/*case ADDR_MODE_*/
					}
				}
			}

			buffer_ptr = c, buffer_ptr++;
			break;
		}

		c++;
	}

proceed:
	/*;*/

/*
	const Instruction **i = NULL;

	const MC6x09_Instruction *i = NULL;
	for (const Instruction **i = g_config.arch->instructions; *i != NULL; i++) {
		if (!((*i)->architectures & FLAG(g_config.arch->value))) {
			goto next_instruction;
		}
		instr_mnem = (*i)->mnemonic;
		line_mnemonic = l->mnemonic;
		while (*instr_mnem != '\0' && *line_mnemonic != '\0') {
			if (*instr_mnem++ != *line_mnemonic++) {
				goto next_instruction;
			}
		}

		switch ((*i)->arg_order) {
		case ARG_ORDER_NONE:
		case ARG_ORDER_INTERREG:
			if (*line_mnemonic != '\0') {
				goto next_instruction;
			}
			if ((*i)->arg_order == ARG_ORDER_NONE) {
				l->address_mode = ADDR_MODE_INHERENT;
				goto instruction_found;
			}

			// check that the two arguments are registers and assign the argument
			if (l->argc != 2) {
				goto next_instruction;
			}
			for (size_t j = 0; j < l->argc; j++) {
				if (l->argv[j].type != ARG_TYPE_UNPROCESSED) {
					goto next_instruction;
				}
				for (reg = &(g_config.arch->registers[1]); reg->name[0] != '\0'; reg++) {
					if (streq(l->argv[j].val.str, reg->name)) {
						l->argv[j].type = ARG_TYPE_REGISTER;
						l->argv[j].val.indexed.base = reg;
						break;
					}
				}
				if (l->argv[j].type != ARG_TYPE_REGISTER) {
					goto next_instruction;
				}
			}
			l->address_mode = ADDR_MODE_INTERREGISTER;
			goto instruction_found;

			break;
		case ARG_ORDER_FROM_REG:
		case ARG_ORDER_TO_REG:
			for (reg = &(g_config.arch->registers[1]); reg->name[0] != '\0'; reg++) {
				if (streq(line_mnemonic, reg->name) &&
					(reg->arcs & FLAG(g_config.arch->value)) &&
					((instruction_reg = instruction_supports_reg(*i, reg)) != NULL)) {
					goto evaluate_args;
					//goto instruction_found;
				}
			}
			goto next_instruction;

evaluate_args:
			if (l->argc < 1) {
				goto next_instruction;
			}
			if (l->argv->type != ARG_TYPE_UNPROCESSED) {
				goto next_instruction;
			}
			argcpy = salloc(sizeof(LineArg));
			memcpy(argcpy, l->argv, sizeof(LineArg));

			char *c = l->argv->val.str;
			char *tmp;
			switch (*c) {
			case '>':
				l->address_mode |= FLAG(ADDR_MODE_EXTENDED);
				c++;
				break;
			case '<':
				l->address_mode |= FLAG(ADDR_MODE_DIRECT);
				c++;
				break;
			case '#':
				l->address_mode |= FLAG(ADDR_MODE_IMMEDIATE);
				c++;
				break;
			case '[':
				l->address_mode |= FLAG(ADDR_MODE_INDIRECT);
				c++;
				for (tmp = c; *tmp != '\0' && *tmp != ']'; tmp++);
				if (*tmp == '\0') {
					fail("Missing terminating ']' character for indirect addressing mode.\n");
				}

				break;
			*/
/*case ',':
				l->address_mode = ADDR_MODE_INDEXED;
				c++;
				break;*//*

			default:
				break;
			}

			printdf("%s\n", c);
			for (; *c != '\0'; c++) {
				switch (*c) {
				case ',':
					if (l->address_mode == ADDR_MODE_INDIRECT) {

					}
					if (l->address_mode != ADDR_MODE_INVALID) {
						fail("Cannot have indexed address mode");
					}
					break;
				//case ''
				}
			}
			//}
			goto instruction_found;
			break;
		}

next_instruction:
		continue;
	}

	fail("Instruction not found.\n");

instruction_found:
	;
	//while (0);

	prepare_line(l);
*/


	data = init_data(salloc(sizeof(Data)));
	/*data = (Data *)salloc_real(sizeof(Data));*/

	/*printdf("Instruction Register is %s\n", instruction_reg ? instruction_reg->reg->name : "NONE");*/
	/*g_config.arch->process_line(l, instruction_reg, data);*/

	add_data(data);
}

static void
prepare_line_att(struct line *l)
{

}

static void
prepare_line_intel(struct line *l)
{

}
