#include <stdint.h>

#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"

void printLexemes(struct LexemeList* list);

int32_t main(int32_t argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return 0;
	}

	char* file;
	{
		FILE* fp;
		fp = fopen(argv[1], "r");

		if (fp == NULL) {
			fprintf(stderr, "failed to read file %s\n", argv[1]);
			return 1;
		}

		fseek(fp, 0L, SEEK_END);
		uint64_t len = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		file = malloc((len + 1) * sizeof(char));

		fread(file, 1, len, fp);
		fclose(fp);
	}

	struct LexemeList* lex_list = lex_code(file);
	// printLexemes(lex_list);

	ParseResult parse;

	parseLex(&parse, lex_list);

	return 0;
}

void printLexemes(struct LexemeList* list) 
{
	const char* strbuff = list->strbuff->data;
	for (uint32_t i = 0; i < list->len; i++)
	{
		const struct Lexeme l = list->data[i];
		switch (l.type)
		{
			case LX_NUM: {
				printf("[%03u:%02u] %-16s: %s\n", l.line, l.col, "NUMBER", strbuff + l.index);
			} break;
			case LX_WORD: {
				printf("[%03u:%02u] %-16s: %s\n", l.line, l.col, "WORD", strbuff + l.index);
			} break;
			default: {
				printf("[%03u:%02u] %-16s: %s\n", l.line, l.col, lx_lookup_names[l.type], lx_lookup_symbols[l.type]);
			}
		}
	}
}