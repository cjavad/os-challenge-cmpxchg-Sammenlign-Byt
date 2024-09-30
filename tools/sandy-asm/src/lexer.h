#pragma once

#include <stdint.h>

extern const char* lx_lookup_names[];
extern const char* lx_lookup_symbols[];

enum LexemeType
{
	LX_DOLLA = 0,
	LX_NOT_A_KILO,
	LX_WORD,
	LX_EMAIL,
	LX_NUM,
	LX_PAREN,
	LX_THESES,
	LX_NOT_A_SHIFT,
	LX_LESS,
	LX_MORE,
	LX_SCENT,
	LX_SQU,
	LX_ARE,
	LX_MOV,
	LX_LARGE_INTESTINE,
	LX_ANOTHA,
	LX_OXFORD
};

struct Lexeme
{
	enum LexemeType type;
	uint32_t line;
	uint32_t col;
	union {
		uint32_t empty;
		uint32_t index;
	};
};
typedef struct Lexeme Lexeme;

struct StringBuffer
{
	uint32_t len;
	uint32_t cap;
	char data[];
};
typedef struct StringBuffer StringBuffer;

struct LexemeList
{
	uint32_t len;
	uint32_t cap;
	StringBuffer* strbuff;
	Lexeme data[];
};
typedef struct LexemeList LexemeList; 

LexemeList* lex_code(const char* code);

const char* lx_lookupName(enum LexemeType type);
const char* lx_lookupSymbol(Lexeme l, StringBuffer* sb);