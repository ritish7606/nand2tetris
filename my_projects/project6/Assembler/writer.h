#ifndef WRITER_H
#define WRITER_H
#include "parser.h"
#include <stdio.h>
typedef struct {
    Parser *parser;
    FILE   *out_fp;
}
Writer;
Writer *writer_create(const char *asm_path, const char *hack_path);
void    writer_write(Writer *w);
void    writer_destroy(Writer *w);
#endif 
