#include "parser.h"
#include "lexer.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "result.h"
ResultDef(Lexeme);

#define error_exit(line, col, fmt, ...) { \
	printf("%s:%u in %s(...) |> ", __FILE_NAME__, __LINE__, __func__); \
	printf("ERROR[%u,%u]: ", line, col); printf(fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
	exit(1); \
}

struct ParseState
{
	uint32_t lex_index;
	LexemeList* ll;
	char errBuff[512];
};
typedef struct ParseState ParseState;

static void lexConsume(ParseState* st);
static bool lexHasNext(ParseState* st);
static Result(Lexeme) lexPeek(ParseState* st);
static Result(Lexeme) tryMatch(ParseState* st, bool (*predicate)(Lexeme));
static Result(Lexeme) tryMatchType(ParseState* st, enum LexemeType type);
static bool matchWord(ParseState* st, const char* word);

static void parseDefine(ParseState* st, ParseResult* result);
static void parseFunc(ParseState* st, ParseResult* result);
static void parseBody(ParseState* st, ParseResult* result);

static uint64_t parseNum(const char* str);

// main parse

void parseLex(ParseResult* result, LexemeList* ll)
{
	result->define_offset = 0;
	result->define_count = 0;
	result->func_count = 0;

	ParseState st = { .lex_index = 0, .ll = ll };
	StringBuffer* sb = ll->strbuff;

	for(;1;) 
	{
		Lexeme l;
		try_unwrap(l, tryMatchType(&st, LX_NOT_A_KILO)) break; catch {
			error_exit(l.line, l.col, "expected \"#\", got \"%s\" (%s)", lx_lookupSymbol(l, sb), lx_lookupName(l.type));
		}
		
		lexConsume(&st);

		// check type
		try_unwrap(l, tryMatchType(&st, LX_WORD)) {
			error_exit(0, 0, "expected LX_WORD, got EOF");
		} catch {
			error_exit(l.line, l.col, "expected LX_WORD, got %s", lx_lookupName(l.type));
		}

		lexConsume(&st);

		// split on word
		if (!strcmp("define", &sb->data[l.index])) {
			parseDefine(&st, result);
		}
		else if (!strcmp("func", &sb->data[l.index])) {
			parseFunc(&st, result);
		}
		else if (!strcmp("body", &sb->data[l.index])) {
			parseBody(&st, result);
		}
		else {
			error_exit(l.line, l.col, "expected \"define\", \"func\", or \"body\", got \"%s\"", &sb->data[l.index]);
		}

	}
}

// ( ⓛ ω ⓛ )

static void parseDefine(ParseState* st, ParseResult* result)
{
	StringBuffer* sb = st->ll->strbuff;

	Lexeme l;

	// get type
	try_unwrap(l, tryMatchType(st, LX_WORD)) {
		error_exit(0, 0, "expected LX_WORD, got EOF");
	} catch {
		error_exit(l.line, l.col, "expected LX_WORD, got %s", lx_lookupName(l.type));
	}
	lexConsume(st);

	uint32_t size = 0;

	if (!strcmp("b", &sb->data[l.index])) {
		size = 1;
	}
	else if (!strcmp("dw", &sb->data[l.index])) {
		size = 4;
	}
	else {
		error_exit(l.line, l.col, "expected \"b\" or \"dw\", got %s", lx_lookupSymbol(l, sb));
	}

	uint32_t offset = (result->define_offset + (size - 1)) & ~(size - 1);

	// get name
	try_unwrap(l, tryMatchType(st, LX_WORD)) {
		error_exit(0, 0, "expected LX_WORD, got EOF");
	} catch {
		error_exit(l.line, l.col, "expected LX_WORD, got %s", lx_lookupName(l.type));
	}

	lexConsume(st);

	const char* name = &sb->data[l.index];

	result->defines[result->define_count].name = name;
	result->defines[result->define_count].offset = offset;

	// loop until #
	for (;1;)
	{
		// ok if no body to define, (peek also doesn't error so not any difference with try_unwrap)
		unwrap(l, lexPeek(st)) {
			goto exit;
		}

		switch (l.type) {
			case LX_ANOTHA: {} break;
			case LX_NOT_A_KILO: {
				goto exit;
			} break;
			case LX_NUM: {
				// parse num
				uint64_t num = parseNum(&sb->data[l.index]);
				// insert into buffer
				switch (size)
				{
					case 1: {
						*(uint8_t*)&(result->define_buffer[offset]) = (uint8_t)num;
					} break;
					case 4: {
						*(uint32_t*)&(result->define_buffer[offset]) = (uint32_t)num;
					} break;
				}
				// add offset
				offset += size;
			} break;
			default: {
				error_exit(l.line, l.col, "expected \"\\n\", \"#\", or number, got %s", lx_lookupSymbol(l, sb));
			} break;
		}

		lexConsume(st);
	}

	exit:
	result->define_offset = offset;
	
	// printf("define %s\noffset %u\n", result->defines[result->define_count].name, result->defines[result->define_count].offset);
	// for (uint32_t i = result->defines[result->define_count].offset; i < offset; i+= size)
	// {
	// 	switch (size) {
	// 		case 1: printf("\t%02x\n", *(uint8_t*)(&result->define_buffer[i])); break;
	// 		case 4: printf("\t%08x\n", *(uint32_t*)(&result->define_buffer[i])); break;
	// 	}
	// }

	result->define_count++;
	return;
}

// (=◉ᆽ◉=)
static void parseFunc(ParseState* st, ParseResult* result)
{
	Lexeme l;
	StringBuffer* sb = st->ll->strbuff;

	// get func name

	try_unwrap(l, tryMatchType(st, LX_WORD)) {
		error_exit(0, 0, "expected LX_WORD, got EOF");
	} catch {
		error_exit(l.line, l.col, "expected LX_WORD, got %s", lx_lookupName(l.type));
	}

	lexConsume(st);

	const char* name = &sb->data[l.index];

	AST_Func* func = &result->functions[result->func_count];
	func->name = name;
	func->args = NULL;
	func->instrs = NULL;

	// read args until newline
	for (bool loop = true; loop;)
	{
		unwrap(l, lexPeek(st)) {
			error_exit(0, 0, "expected word or newline, got EOF");
		}

		lexConsume(st);

		switch (l.type)
		{
			case LX_ANOTHA: {
				loop = false;
			} break;
			case LX_WORD: {
				// read arg in (yarg) ((pirate sound funny))
				const char* arg_name = &sb->data[l.index];
				AST_Arg** arg_ptr = &func->args;
				while (!(*arg_ptr)) {
					arg_ptr = &((*arg_ptr)->next);
				}
				*arg_ptr = malloc(sizeof(AST_Arg));

				(*arg_ptr)->next = NULL;
				(*arg_ptr)->name = arg_name;
			} break;
			case LX_OXFORD: {} break; // lmao whitespace
			default: {
				error_exit(l.line, l.col, "expected word, newline, or comma, got %s", lx_lookupName(l.type));
			}
		}
	}

	// read instructions until new

	for (bool loop = true; loop;)
	{
		unwrap(l, lexPeek(st)) break;

		// got instruction/call
		
		// parse instruction until newline
		// inline if function call
	}

	exit:
	result->func_count++;
	return;
}

// ( ⓛ Ⱉ ⓛ )
static void parseBody(ParseState* st, ParseResult* result)
{

}

// +==============+
// | HELPER FUNCS |
// +==============+

static void lexConsume(ParseState* st)
{
	st->lex_index++;
}

static bool lexHasNext(ParseState* st)
{
	return st->lex_index < st->ll->len;
}

static Result(Lexeme) lexPeek(ParseState* st)
{
	if (st->lex_index == st->ll->len) {
		return  None(Lexeme);
	}

	Lexeme l = st->ll->data[st->lex_index];
	return  Ok(Lexeme, l);
}

static Result(Lexeme) tryMatch(ParseState* st, bool (*predicate)(Lexeme))
{
	Lexeme l;
	unwrap(l, lexPeek(st)) return None(Lexeme);

	if (predicate(l)) {
		return Ok(Lexeme, l);
	}
	return Err(Lexeme, l);
}

static Result(Lexeme) tryMatchType(ParseState* st, enum LexemeType type)
{
	return tryMatch(st, lambda(bool, (Lexeme l){
		return l.type == type;
	}));
}

static bool matchWord(ParseState* st, const char* word)
{
	const char* sb = st->ll->strbuff->data;
	Lexeme l;
	unwrap(l, tryMatchType(st, LX_WORD)) return false;
	return !strcmp(&sb[l.index], word);
}

static uint64_t parseNum(const char* str)
{
	if (*str == '0' && *(str + 1) == 'x')
	{
		// hex
		str+=2;
		uint64_t num = 0;
		while (*str)
		{
			num = (num * 16) + (*str <= '9' ? *str - '0' : *str <= 'F' ? *str - 'A' + 10 : *str - 'a' + 10);
			str++;
		}
		return num;
	}
	else 
	{
		// dec
		uint64_t num = 0;
		while (*str)
		{
			num = (num * 10) + *str - '0';
			str++;
		}
		return num;
	}
}