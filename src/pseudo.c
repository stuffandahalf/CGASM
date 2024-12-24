#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#define GASMIC_CAN_SWITCH_DIR	1
#elif defined(_WIN32)
#include <direct.h>
#define chdir	_chdir
#define GASMIC_CAN_SWITCH_DIR	1
#endif

#if defined(HAVE_FCNTL_H) && defined(HAVE_SYS_STAT_H)
#include <fcntl.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <io.h>
#if !defined(open)
#define open	_open
#endif /* !defined(open) */
#if !defined(close)
#define close	_close
#endif /* !defined(close) */
#if !defined(read)
#define read	_read
#endif /* !defined(read) */
#define stat	_stat
#define fstat	_fstat
#endif	/* defined(_WIN32) */
#define GASMIC_HAVE_POSIX_FILE_IO	1
#endif	/* defined(HAVE_FCNTL_H) && defined(HAVE_SYS_STAT_H) */

#include "as.h"
#include "pseudo.h"
#include "arithmetic.h"
#include "lang.h"
#include "smem.h"

/*
 * struc?
 * .org
 * .equ
 * .include
 * .org
 * .resb
 * .fcb
 * .extern
 * .global
 * .ascii
 * .asciz
 * .extern
 * .global
 */

static void pseudo_set_file(struct line *line);
static void pseudo_set_arch(struct line *line);
static void pseudo_set_byte(struct line *line);
static void pseudo_set_word(struct line *line);
static void pseudo_set_double(struct line *line);
static void pseudo_set_quad(struct line *line);
static void pseudo_reserve_bytes(struct line *line);
static void pseudo_reserve_words(struct line *line);
static void pseudo_reserve_doubles(struct line *line);
static void pseudo_reserve_quads(struct line *line);
static void pseudo_equ(struct line *line);
static void pseudo_include(struct line *line);
static void pseudo_insert(struct line *line);
static void pseudo_org(struct line *line);

static struct pseudo_instruction pseudo_ops[] = {
	{ ".ARCH",		&pseudo_set_arch,			1 },
	{ ".FILE",		&pseudo_set_file,			1 },

	{ ".DB",		&pseudo_set_byte,			-1 },
	{ ".DW",		&pseudo_set_word,			-1 },
	{ ".DD",		&pseudo_set_double,			-1 },
	{ ".DQ",		&pseudo_set_quad,			-1 },

	{ ".RESB",		&pseudo_reserve_bytes,		1 },
	{ ".RESW",		&pseudo_reserve_words,		1 },
	{ ".RESD",		&pseudo_reserve_doubles,	1 },
	{ ".RESQ",		&pseudo_reserve_quads,		1 },

	{ ".EQU",		&pseudo_equ,				1 },
	{ ".INCLUDE",	&pseudo_include,			1 },
	{ ".INSERT",	&pseudo_insert,				1 },
	{ ".ORG",		&pseudo_org,				1 },
	/*{ ".SYNTAX", &pseudo_syntax, 1 },*/
	{ NULL,			NULL,						0 }
};

struct pseudo_instruction *
get_pseudo_op(struct line *line)
{
	struct pseudo_instruction *pseudo_op;

	for (pseudo_op = pseudo_ops; pseudo_op->instruction != NULL; pseudo_op++) {
		if ((pseudo_op->args == -1 || pseudo_ops->args == line->argc) &&
			streq(line->mnemonic, (*line->mnemonic == '.' ? pseudo_op->instruction : &pseudo_op->instruction[1]))) {
			return pseudo_op;
		}
	}
	return NULL;
}

void
parse_pseudo_op(struct line *line)
{
	struct pseudo_instruction *pseudo_inst = get_pseudo_op(line);
	if (pseudo_inst == NULL) {
		fail("Unable to find pseudo instruction %s that takes " SZuFMT " arguments.\n", line->mnemonic, line->argc);
	}
	pseudo_inst->process(line);
}

static void
pseudo_set_arch(struct line *line)
{
	const Architecture *arch;

	if (datatab->first != NULL) {
		fail("Cannot switch architecture after code.\n");
	}

	arch = find_arch(line->argv[0].val.str);
	if (arch == NULL) {
		fail("Failed to locate architecture %s.\n", line->argv[0].val.str);
	}
	g_config.arch = arch;
	init_address_mask();
	printdf(("%s\n", g_config.arch->name));
}

static void
pseudo_set_file(struct line *line)
{
	if (line->argv[0].type != ARG_TYPE_STRING) {
		die("File name must be a string.");
	}
	sfree(g_context->fname);
	if ((g_context->fname = saquire(str_clone(line->argv[0].val.str))) == NULL) {
		fail("Failed to copy substitute file name.\n");
	}
}

#define pseudo_set_data(T, line) { \
	Data *data; \
	size_t i; \
	for (i = 0; i < (line)->argc; i++) { \
		data = init_data(salloc(sizeof(Data))); \
		data->address = address & address_mask; \
		if ((line)->argv[i].type == ARG_TYPE_STRING) { \
			data->type = DATA_TYPE_BYTES; \
			data->bytec = strlen((line)->argv[i].val.str); \
			data->contents.bytes = salloc(sizeof(uint8_t) * data->bytec); \
			memcpy(data->contents.bytes, (line)->argv[i].val.str, data->bytec); \
		} else { \
			data->type = DATA_TYPE_EXPRESSION; \
			data->bytec = sizeof(T); \
			data->contents.rpn_expr = (line)->argv[i].val.rpn_expr; \
		} \
		address += data->bytec; \
		add_data(data); \
	} \
}

static void pseudo_set_byte(struct line *line) { pseudo_set_data(uint8_t, line); }
static void pseudo_set_word(struct line *line) { pseudo_set_data(uint16_t, line); }
static void pseudo_set_double(struct line *line) { pseudo_set_data(uint32_t, line); }
static void pseudo_set_quad(struct line *line) { pseudo_set_data(uint64_t, line); }

#undef pseudo_set_data

/* TODO: Need to resolve this */
#define pseudo_reserve_data(T, line) { \
	/*if (line->argc != 1) { \
		fail("Reserving bytes requires one parameter.\n"); \
	} \*/ \
	long count = 0; \
	char *end; \
	Data *data = init_data(salloc(sizeof(Data))); \
	data->type = DATA_TYPE_BYTES; \
}

static void pseudo_reserve_bytes(struct line *line) { pseudo_reserve_data(uint8_t, line); }
static void pseudo_reserve_words(struct line *line) { pseudo_reserve_data(uint16_t, line); }
static void pseudo_reserve_doubles(struct line *line) { pseudo_reserve_data(uint32_t, line); }
static void pseudo_reserve_quads(struct line *line) { pseudo_reserve_data(uint64_t, line); }

#undef pseudo_reserve_data

static void
pseudo_equ(struct line *line)
{
	char *num_end;

	if (!(line->line_state & FLAG(LINE_STATE_LABEL))) {
		fail("Pseudo instruction .EQU requires a label on the same line.\n");
	}
	if (line->argc != 1) {
		fail("Invalid number of arguments for pseudo instruction .EQU.\n");
	}

	/* TODO: replace this with rpn arithmetic parsing */
	symtab.last->value = strtol(line->argv[0].val.str, &num_end, 0);
	/*if (line->argv[0] == num_end) {*/
	if (*num_end != '\0') {
		fail("Failed to parse given value.\n");
	}
}

static void
pseudo_include(struct line *line)
{
	struct context included_context;

	if (line->argv[0].type != ARG_TYPE_STRING) {
		fail("File name is not a string. Did you forget to surround the file name in quotes?\n");
	}

	included_context.parent = g_context;
	/*included_context.fptr = included_file; */
	included_context.line_num = 1;
	if ((included_context.fname = saquire(str_clone(line->argv[0].val.str))) == NULL) {
		fail("Failed to duplicate file name \"%s\"\n", line->argv[0].val.str);
	}

	/*FILE *included_file;*/
	if ((included_context.fptr = fopen(included_context.fname, "r")) == NULL) {
		fail("Failed to open included file \"%s\".\n", included_context.fname);
	}

	g_context = &included_context;

	assemble(&included_context);
	fclose(included_context.fptr);

	g_context = g_context->parent;
	sfree(included_context.fname);

	/* need to reassign argv because assemble frees it but we return back to assemble. */
	/*line->argc = 1; */
	/*line->argv = salloc(line->argc * sizeof(char *));*/
}

/*
 * Inserts the raw bytes of this file into the resulting binary
 */
static void
pseudo_insert(struct line *line)
{
	Data *file_data;
#if defined(GASMIC_HAVE_POSIX_FILE_IO)
	int fd;
	off_t size;
	struct stat fstatus;

	if (line->argv[0].type != ARG_TYPE_STRING) {
		fail("Inserted file argument is not a string path.\n");
	}

	fd = open(line->argv[0].val.str, O_RDONLY);
	if (fd < 0) {
		fail("Failed to open file.\n");
	}

	if (fstat(fd, &fstatus) < 0) {
		fail("Failed to get file specifications.\n");
	}

	size = fstatus.st_size;
#else /* defined(GASMIC_HAVE_POSIX_FILE_IO) */
	FILE *inserted_file;
	long size;

	if (line->argv[0].type != ARG_TYPE_STRING) {
		fail("Inserted file argument is not a string path.\n");
	}

	inserted_file = fopen(line->argv[0].val.str, "rb");
	if (inserted_file == NULL) {
		fail("Failed to open file \"%s\" to be inserted. Does the file exists?\n", line->argv[0].val.str);
	}

	size = fsize(inserted_file);
#endif /* defined(GASMIC_HAVE_POSIX_FILE_IO) */

	while (size > 0) {
		file_data = init_data(salloc(sizeof(Data)));
		file_data->bytec = (uint8_t)((size > 255) ? 255 : size);
		file_data->address = address;
		file_data->type = DATA_TYPE_BYTES;
		file_data->contents.bytes = salloc(sizeof(uint8_t) * file_data->bytec);

#if defined(GASMIC_HAVE_POSIX_FILE_IO)
		read(fd, file_data->contents.bytes, file_data->bytec);
#else
		fread(file_data->contents.bytes, sizeof(uint8_t), file_data->bytec, inserted_file);
#endif

		add_data(file_data);

		address += file_data->bytec;
		size -= file_data->bytec;
	}

#if defined(GASMIC_HAVE_POSIX_FILE_IO)
	close(fd);
#else
	fclose(inserted_file);
#endif /* defined(GASMIC_HAVE_POSIX_FILE_IO) */

	printdf(("Inserted fname is %s\n", line->argv[0].val.str));
}

static void
pseudo_org(struct line *line)
{
	char *lend;
	size_t new_address = strtoul(line->argv[0].val.str, &lend, 0) & address_mask;
	if (*lend == '\0') {
		/*printdf(("new address is 0xzX\n", new_address));*/
		printdf(("new address is 0x" SZXFMT "\n", new_address));
		address = new_address;
	} else {
		fail("Value is not a number.\n");
	}
}

