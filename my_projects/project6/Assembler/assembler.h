#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include "writer.h"
typedef struct {
    char   *asm_path;
    char   *hack_path;
    Writer *writer;
} Assembler;
Assembler *assembler_create(const char *asm_path);
void assembler_compile(Assembler *a);
void assembler_destroy(Assembler *a);
#endif 