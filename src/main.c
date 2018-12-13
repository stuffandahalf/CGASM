#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <as.h>
#include <arch.h>

#define LINEBUFFERSIZE 256

char buffer[LINEBUFFERSIZE];

void configure(int argc, char *argv[]);
static Architecture *str_to_arch(const char arch_name[]);
static char *str_to_upper(char str[]);
Instruction *find_instruction(const char *mnem);

char *out_fname;
FILE *in;
FILE *out;
Architecture *arch;
size_t address;

int main(int argc, char **argv) {
    out_fname = strdup("a.out");
    in = stdin;
    //out = stdout;
    arch = architectures;
    address = 0;

    configure(argc, argv);

    //out = fopen(out_fname, "w+");
    free(out_fname);
    
    while (fgets(buffer, LINEBUFFERSIZE, in) != NULL) {
        str_to_upper(buffer);
        //Instruction *i = find_instruction(str_to_upper(buffer));
        fprintf(out, "buffer after conversion contains %s\n", buffer);
        
        
        Instruction *i = find_instruction(buffer);
        if (i != NULL) {
            fprintf(out, "%s, %X, %X\n", i->mne, i->base_opcode, i->regs);
        }
    }
    
    //fclose(out);

    /*printf("%s\n", find_instruction("ADC")->mne);
    printf("%s\n", arch->name);*/
    
	return 0;
}

void configure(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "m:o:f:")) != -1) {
        switch (c) {
            case 'm':
                arch = str_to_arch(optarg);
                if (arch == NULL) {
                    die("Unsupported architecture: %s\n", optarg);
                }
                break;
            case 'o':
                free(out_fname);
                out_fname = strdup(optarg);
                break;
            case 'f':
                break;
            case 0:
                break;
        }
    }
}

static Architecture *str_to_arch(const char arch_name[]) {
    for (Architecture *a = architectures; a->name[0] != '\0'; a++) {
        if (streq(arch_name, a->name)) {
            return a;
        }
    }
    return NULL;
}

static char *str_to_upper(char *str) {
    char *c;
    for (c = str; *c != '\0'; c++) {
        if (*c >= 'a' && *c <= 'z') {
            *c += 'A' - 'a';
        }
        else if (*c == '\n') {
            *c = '\0';
        }
    }
    return str;
}

Instruction *find_instruction(const char *mnem) {
    for (Instruction *i = instructions; i->mne[0] != '\0'; i++) {
        printf("%s\t%s\n", mnem, i->mne);
        if (streq(mnem, i->mne)) {
            return i;
        }
    }
    return NULL;
}
