#include "lexer.h"

const char* lx_lookup_names[] = {
	"DOLLA",
	"NOT A KILO",
	"WORD",
	"EMAIL",
	"NUM",
	"PAREN",
	"THESES",
	"NOT A SHIFT",
	"LESS",
	"MORE",
	"SCENT",
	"SQU",
	"ARE",
	"MOV",
	"LARGE INTESTINE",
	"ANOTHA",
	"OXFORD"
};

const char* lx_lookup_symbols[] = {
	"$",
	"#",
	"E!R!R!O!R",
	"@",
	"E!R!R!O!R",
	"(",
	")",
	"*",
	"-",
	"+",
	"%",
	"[",
	"]",
	"=",
	":",
	"\\n",
	","
};

const char* lx_lookupName(enum LexemeType type) {
	return lx_lookup_names[type];
}

const char* lx_lookupSymbol(Lexeme l, StringBuffer* sb) {
	if (l.type == LX_WORD || l.type == LX_NUM) {
		return &sb->data[l.index];
	} else {
		return lx_lookup_symbols[l.type];
	}
}