#include <stdarg.h>

#include "as.h"
#include "ansistyle.h"
#include "arithmetic.h"
#include "console.h"
#include "smem.h"
#include "lang.h"

extern Architecture **architectures[];

struct symboltab symtab = {
	.first = NULL,
	.last = NULL
};
DataTab *datatab;

/*
 * Resolve a label into it's complete representation
 * returns a salloced char *.
 */
char *
resolve_label(char *symbol)
{
	size_t parent_len, local_len;
	char *symbol_ptr;
	char *complete_symbol = NULL;
	char *c;

	if (*symbol != '.') {
		complete_symbol = salloc(sizeof(char) * (strlen(symbol) + 1));
		strcpy(complete_symbol, symbol);

		return complete_symbol;
	}

	if (symtab.last_parent == NULL) {
		fail("Local label cannot be defined before any non-local labels.\n");
	}

	parent_len = strlen(symtab.last_parent->label);
	local_len = strlen(symbol);
	complete_symbol = salloc(sizeof(char) * (parent_len + local_len + 1));

	symbol_ptr = complete_symbol;
	for (c = symtab.last_parent->label; *c != '\0'; c++) {
		*symbol_ptr++ = *c;
	}
	for (c = symbol; *c != '\0'; c++) {
		*symbol_ptr++ = *c;
	}
	*symbol_ptr = '\0';
	printdf(("Symbol \"%s\" resolved to \"%s\"\n", symbol, complete_symbol));

	return complete_symbol;
}


void
add_label(struct line *line)
{
	struct symbol *test_sym;

	struct symbol *sym = salloc(sizeof(struct symbol));

	sym->next = NULL;
	sym->label = resolve_label(line->label);

	if (line->label[0] == '.') {
		line->label = sym->label;
	} else {
		symtab.last_parent = sym;
	}

	/*if (line->label[0] == '.') {
		if (symtab.last == NULL) {
			fail("Local label cannot be defined before any non-local labels.\n");
		}
		sym->label = salloc(sizeof(char) * (strlen(symtab.last_parent->label) + strlen(line->label) + 1));
		register char *label = sym->label;
		register char *c;
		for (c = symtab.last_parent->label; *c != '\0'; c++) {
			*(label++) = *c;
		}
		for (c = line->label; *c != '\0'; c++) {
			*(label++) = *c;
		}
		*label = '\0';
		line->label = sym->label;
	}
	else {
		sym->label = salloc(strlen(line->label) + sizeof(char));
		strcpy(sym->label, line->label);
		symtab.last_parent = sym;
	}*/

	/*printdf("Parsed label = %s\n", sym->label);*/

	sym->value = address;

	for (test_sym = symtab.first; test_sym != NULL; test_sym = test_sym->next) {
		if (streq(test_sym->label, sym->label)) {
			fail("Symbol \"%s\" is already defined.\n", test_sym->label);
		}
	}

	if (symtab.first == NULL) {
		symtab.first = sym;
	} else {
		symtab.last->next = sym;
	}
	symtab.last = sym;
}

int
init_data_table()
{
	if (datatab != NULL) {
		return 0;
	}
	datatab = salloc(sizeof(DataTab));
	datatab->first = NULL;
	datatab->last = NULL;

	return 1;
}

Data *
init_data(Data *data)
{
	data->next = NULL;
	data->type = DATA_TYPE_NONE;
	data->bytec = 0;
	data->contents.bytes = NULL;
	return data;
}

void
add_data(Data *data)
{
	Data *next;

	if (datatab->first == NULL) {
		datatab->first = data;
	} else {
		datatab->last->next = data;
	}
	datatab->last = data;
	next = datatab->last->next;
	while (next != NULL) {
		datatab->last = next;
		next = next->next;
	}
}

const Architecture *
find_arch(const char *arch_name)
{
	Architecture ***a = NULL;
	for (a = architectures; *a != NULL; a++) {
		if (streq(arch_name, (**a)->name)) {
			return **a;
		}
	}
	return NULL;
}

const Register *
find_reg(const char *name)
{
	const Register *r = NULL;

	for (r = g_config.arch->registers; *r->name != '\0'; r++) {
		if (streq(name, r->name)) {
			return r;
		}
	}
	return NULL;
}

void
prepare_line(struct line *line)
{
#if 1
	size_t i;
	struct token *tok;

	for (i = 0; i < line->argc; i++) {
		if (line->argv[i].type == ARG_TYPE_UNPROCESSED) {
			line->argv[i].type = ARG_TYPE_EXPRESSION;
			line->argv[i].val.rpn_expr = parse_expression(line->argv[i].val.str);
			if (arithmetic_status_code != ARITHMETIC_SUCCESS) {
				fail("Failed to parse expression.");
			}
			for (tok = line->argv[i].val.rpn_expr; tok != NULL; tok = tok->next) {
				if (tok->type == TOKEN_TYPE_SYMBOL && tok->value.str[0] == '.') {
					size_t parent_size = strlen(symtab.last_parent->label);
					size_t child_size = strlen(tok->value.str);
					char *child = tok->value.str;

					tok->value.str = malloc(sizeof(char) * (parent_size + child_size + 1));
					if (tok->value.str == NULL) {
						fail("Failed to reallocate buffer for local symbol in expression.\n");
					}
					strncpy(tok->value.str, symtab.last_parent->label, parent_size);
					strncpy(&tok->value.str[parent_size], child, child_size + 1);
					free(child);
				}
			}
		}
	}
#endif
}

NORETURN void
fail(const char *fmt, ...)
{
	struct context *cntxt;
	va_list args;
	va_start(args, fmt);
	printef("[%s:" SZuFMT "]\t", g_context->fname, g_context->line_num);

#ifdef _WIN32
	if (console_is_init()) {
#endif
	printef(ANSI_COLOR_RED);
#ifdef _WIN32
	}
#endif
	printef("ERROR");
#ifdef _WIN32
	if (console_is_init()) {
#endif
	printef(ANSI_COLOR_RESET);
#ifdef _WIN32
	}
#endif
	printef(" ");

	vfprintf(stderr, fmt, args);
	va_end(args);

	for (cntxt = g_context->parent; cntxt != NULL; cntxt = cntxt->parent) {
		printef("\tIn file included from \"%s\", line " SZuFMT "\n", cntxt->fname, cntxt->line_num);
	}

#ifdef _WIN32
	restore_console();
#endif

	die("\n");
}
