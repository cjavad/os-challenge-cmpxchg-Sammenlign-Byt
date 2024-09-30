#pragma once

#include <stdint.h>

#include "lexer.h"

struct AST_Define {
	const char* name;
	uint32_t offset;
};
typedef struct AST_Define AST_Define;

struct AST_Func {
	const char* name;
	struct AST_Arg* args;
	struct AST_Instruction* instrs;
};
typedef struct AST_Func AST_Func;

struct AST_Arg {
	const char* name;
	struct AST_Arg* next;
};
typedef struct AST_Arg AST_Arg;

struct AST_Body {
	struct AST_Instruction* instrs;
};
typedef struct AST_Body AST_Body;

struct AST_Instruction {
	const char* name;
	struct AST_Expr* exprs;
	struct AST_Instruction* next;
};
typedef struct AST_Instruction AST_Instruction;

struct AST_Expr {
	enum {
		MUL,
		SUB,
		ADD,
		NUM,
		VAR,
		ARG
	} type;
	union {
		struct {struct Expr* left; struct Expr* right;} MUL;
		struct {struct Expr* left; struct Expr* right;} SUB;
		struct {struct Expr* left; struct Expr* right;} ADD;
		struct {uint64_t num;} NUM;
		struct {const char* name;} VAR;
		struct {const char* name;} ARG;
	};
};
typedef struct AST_Expr AST_Expr;

struct ParseResult
{
	uint32_t define_offset;
	uint32_t define_count;
	uint32_t func_count;

	// these hard coded limits will never become a problem
	// promise
	AST_Define defines[64];
	AST_Func functions[64];
	AST_Body body;

	// will never fill up (i swear)
	uint8_t define_buffer[16384];
};
typedef struct ParseResult ParseResult;

void parseLex(ParseResult* result, LexemeList* ll);