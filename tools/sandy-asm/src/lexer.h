#pragma once

#include <stdint.h>

extern const char* lx_lookup_name[];
extern const char* lx_lookup_symbols[];

enum LexemeType
{
	LX_DOLLA,
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

struct StringBuffer
{
	uint32_t len;
	uint32_t cap;
	char data[];
};

struct LexemeList
{
	uint32_t len;
	uint32_t cap;
	struct StringBuffer* strbuff;
	struct Lexeme data[];
};

struct LexemeList* lex_code(const char* code);