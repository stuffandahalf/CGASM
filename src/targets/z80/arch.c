#include "arch.h"

Architecture *ARCH_Z80;
Architecture *ARCH_I8080;

static const Register registers[] = {
	{ "A", 1, Z80 },
	{ "F", 1, Z80 },
	{ "AF", 2, Z80 },
	{ "B", 1, Z80 },
	{ "C", 1, Z80 },
	{ "BC", 2, Z80 },
	{ "D", 1, Z80 },
	{ "E", 1, Z80 },
	{ "DE", 2, Z80 },
	{ "H", 1, Z80 },
	{ "L", 1, Z80 },
	{ "HL", 2, Z80 },
	{ "IX", 2, Z80 },
	{ "IY", 2, Z80 },
	{ "SP", 2, Z80 },
	{ "I", 1, Z80 },
	{ "R", 1, Z80 },
	{ "PC", 2, Z80 },
	{ "", 0, 0 }
};
#ifndef NDEBUG
static const int regc = sizeof(registers) / sizeof(Register) - 1;
#endif

#define Z80_REGISTER(reg) (((reg) >= 0 && (reg) <= Z80_REG_PC) ? &(registers[reg]) : NULL)

static const Instruction I_NOP = {
	"NOP",
	Z80,
	ARG_ORDER_NONE,
	{
		{
			Z80_REGISTER(Z80_REG_NONE),
			{
				{ ADDR_MODE_INHERENT, 1, 0x00 },
				{ ADDR_MODE_INVALID, 0, 0 }
			}
		},
		{ NULL, 0 }
	}
};

static const Instruction *instructions[] = {
	&I_NOP,
	NULL
};

void
Z80_process_line(struct line *line, const struct instruction_register *instr_reg, Data *data)
{
	printdf(("Hello from Z80 line processor.\n"));
}

void
Z80_init(void)
{
	ARCH_Z80 = salloc(sizeof(Architecture));
	ARCH_Z80->value = Z80;
	/*strcpy(ARCH_Z80->name, "z80");*/
	ARCH_Z80->name = "Z80";
	ARCH_Z80->byte_size = 8;
	ARCH_Z80->bytes_per_address = 2;
	ARCH_Z80->endianness = ARCH_ENDIAN_LITTLE;
	ARCH_Z80->default_syntax = SYNTAX_INTEL;
	ARCH_Z80->registers = registers;
	ARCH_Z80->instructions = instructions;
	ARCH_Z80->process_line = &Z80_process_line;
}
void
Z80_destroy(void)
{
	sfree(ARCH_Z80);
}

void
I8080_init(void)
{
	fail("I8080 architecture is not ready yet.");
	ARCH_I8080 = salloc(sizeof(Architecture));
}
void
I8080_destroy(void)
{
	sfree(ARCH_I8080);
}
