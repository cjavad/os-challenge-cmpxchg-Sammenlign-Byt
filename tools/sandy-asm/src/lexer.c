#include "lexer.h"

#include <stdlib.h>
#include <stdio.h>

#define SIMPLE_LEXEME(T) ll_push(&lexemes, (struct Lexeme){.type = T, .line = line, .col = col})

static void sb_init(struct StringBuffer** buff);
static void sb_push(struct StringBuffer** buff, const char c);

static void ll_init(struct LexemeList** list);
static void ll_push(struct LexemeList** list, const struct Lexeme l);

static void error_exit(uint32_t line, uint32_t col, const char* message);

struct LexemeList* lex_code(const char* code)
{
	struct LexemeList* lexemes;
	ll_init(&lexemes);
	sb_init(&(lexemes->strbuff));

	uint32_t line = 1;
	uint32_t col = 1;

	uint32_t err_line;
	uint32_t err_col;

	enum {
		NORMAL,
		NUMBER,
		NUMBER_HEX,
		WORD,
		COMMENT
	} state = NORMAL;

	char c;
	while (c = *code)
	{
		switch(state)
		{
			case NORMAL: {
				switch (c)
				{
					case '$': {SIMPLE_LEXEME(LX_DOLLA);} break;
					case '#': {SIMPLE_LEXEME(LX_NOT_A_KILO);} break;
					case '@': {SIMPLE_LEXEME(LX_EMAIL);} break;
					case '(': {SIMPLE_LEXEME(LX_PAREN);} break;
					case ')': {SIMPLE_LEXEME(LX_THESES);} break;
					case '*': {SIMPLE_LEXEME(LX_NOT_A_SHIFT);} break;
					case '-': {SIMPLE_LEXEME(LX_LESS);} break;
					case '+': {SIMPLE_LEXEME(LX_MORE);} break;
					case '%': {SIMPLE_LEXEME(LX_SCENT);} break;
					case '[': {SIMPLE_LEXEME(LX_SQU);} break;
					case ']': {SIMPLE_LEXEME(LX_ARE);} break;
					case '=': {SIMPLE_LEXEME(LX_MOV);} break;
					case ':': {SIMPLE_LEXEME(LX_LARGE_INTESTINE);} break;
					
					case '\n': {
						if (lexemes->len > 0 && lexemes->data[lexemes->len - 1].type != LX_ANOTHA) {
							SIMPLE_LEXEME(LX_ANOTHA);
						}
						line++; col = 0;
					} break;
					
					case '/': {
						if (*(code + 1) != '/') {
							error_exit(line, col, "oopsie dasies, you started your comment wrong");
						}
						state = COMMENT;
						code++; col++;
					} break;

					case 'a' ... 'z':
					case 'A' ... 'Z': {
						ll_push(&lexemes, (struct Lexeme){.type = LX_WORD, .line = line, .col = col, .index = lexemes->strbuff->len});
						sb_push(&(lexemes->strbuff), c);
						err_line = line;
						err_col = col;
						state = WORD;
					} break;

					case '0' ... '9': {
						ll_push(&lexemes, (struct Lexeme){.type = LX_NUM, .line = line, .col = col, .index = lexemes->strbuff->len});
						sb_push(&(lexemes->strbuff), c);
						err_line = line;
						err_col = col;
						state = NUMBER;

						if (*(code + 1) == 'x') {
							code++; col++;
							sb_push(&(lexemes->strbuff), 'x');
							state = NUMBER_HEX;
						}
					} break;

					case ' ':
					case ',':
					case '\t': {
						// eeeeeeeeeeeeeeeyyyyyyyo
					
						// comma is whitespace now
					} break;

					default: {
						printf("arghehhhgehgheg: %c\n", c);
						error_exit(line, col, "uh oh, that shouldn't happen");
					}
				}
			} break;
			case NUMBER: {
				switch (c)
				{
					case '0' ... '9': {
						sb_push(&(lexemes->strbuff), c);
					} break;
					default : {
						code--; col--;
						sb_push(&(lexemes->strbuff), 0);	
						state = NORMAL;
					}
				}
			} break;
			case NUMBER_HEX: {
				switch (c)
				{
					case 'A' ... 'F':
					case 'a' ... 'f':
					case '0' ... '9': {
						sb_push(&(lexemes->strbuff), c);
					} break;
					default: {
						code--; col--;
						sb_push(&(lexemes->strbuff), 0);
						state = NORMAL;
					}
				}
			} break;
			case WORD: {
				switch (c)
				{
					case 'a' ... 'z':
					case 'A' ... 'Z': 
					case '0' ... '9': {
						sb_push(&(lexemes->strbuff), c);
					} break;
					default: {
						code--; col--;
						sb_push(&(lexemes->strbuff), 0);
						state = NORMAL;
					}
				}
			} break;
			case COMMENT: {
				if (c == '\n') {
					code--; col--;
					state = NORMAL;
				}
			} break;
			default: {
				error_exit(line, col, "divine intervention by cosmic ray");
			}
		}
		col++;
		code++;
	}

	return lexemes;
}

static void error_exit(uint32_t line, uint32_t col, const char* message)
{
	printf("ERROR[%u,%u]: %s\n", line, col, message);
	exit(1);
}

static void sb_init(struct StringBuffer** buff)
{
	(*buff) = malloc(sizeof(struct StringBuffer) + sizeof(char) * 1024);
	(*buff)->len = 0;
	(*buff)->cap = 1024;
}

static void sb_push(struct StringBuffer** buff, const char c)
{
	if ((*buff)->len == (*buff)->cap) {
		(*buff)->cap = (*buff)->cap << 1;
		(*buff) = realloc((*buff), sizeof(struct StringBuffer) + sizeof(char) * (*buff)->cap);
	}
	(*buff)->data[(*buff)->len] = c;
	(*buff)->len = (*buff)->len + 1;
}

static void ll_init(struct LexemeList** list)
{
	(*list) = malloc(sizeof(struct LexemeList) + sizeof(struct Lexeme) * 256);
	(*list)->len = 0;
	(*list)->cap = 256;
}

static void ll_push(struct LexemeList** list, const struct Lexeme l)
{
	if ((*list)->len == (*list)->cap) {
		(*list)->cap = (*list)->cap << 1;
		(*list) = realloc((*list), sizeof(struct LexemeList) + sizeof(struct Lexeme) * (*list)->cap);
	}
	(*list)->data[(*list)->len] = l;
	(*list)->len = (*list)->len + 1;
}