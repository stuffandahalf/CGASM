#ifdef _WIN32
#include <stdio.h>
#include <Windows.h>

#include "ansistyle.h"
#include "console.h"

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif /* !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING) */

static int init;
static HANDLE stderr_handle;
static DWORD stderr_mode_default;

int
console_is_init(void)
{
	return init;
}

int
init_console(void)
{
	DWORD stderr_mode;

	init = 0;
	stderr_mode = 0;
	stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
	if (stderr_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	if (!GetConsoleMode(stderr_handle, &stderr_mode)) {
		return 0;
	}

	stderr_mode_default = stderr_mode;
	stderr_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	if (!SetConsoleMode(stderr_handle, stderr_mode)) {
		return 0;
	}

	init = 1;
	return 1;
}

int
restore_console(void)
{
	if (init) {
		fprintf(stderr, ANSI_COLOR_RESET);
		if (!SetConsoleMode(stderr_handle, stderr_mode_default)) {
			return 0;
		}
	}
	
	return 1;
}

#endif /* defined(_WIN32) */

