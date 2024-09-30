#pragma once

enum ResultStatus
{
	RESULT_OK,
	RESULT_NONE,
	RESULT_ERR
};

#define Result(type) struct _RESULT__##type

// all ok
#define Ok(type, v) (Result(type)){ .status = RESULT_OK, .value = v }
// nothing
#define None(type) (Result(type)){ .status = RESULT_NONE }
// something went wrong
#define Err(type, v) (Result(type)){ .status = RESULT_ERR, .value = v }

#define ResultDef(type) Result(type) { enum ResultStatus status; type value; }

#define ____CONCAT_IMPL(a, b) a##b
#define ____CONCAT(a, b) ____CONCAT_IMPL(a, b)

#define ____UNWRAP_IMPL(dest, result, iii) \
	__auto_type ____CONCAT(_RESULT__VAR, iii) = result; \
	if (____CONCAT(_RESULT__VAR, iii).status == RESULT_OK) { dest = ____CONCAT(_RESULT__VAR, iii).value; }\
	else
#define unwrap(dest, result) ____UNWRAP_IMPL(dest, result, __COUNTER__)

#define ____TRY_UNWRAP_IMPL(dest, result, iii) \
	__auto_type ____CONCAT(_RESULT__VAR, iii) = result; \
	if (____CONCAT(_RESULT__VAR, iii).status != RESULT_NONE) { dest = ____CONCAT(_RESULT__VAR, iii).value; } \
	if (____CONCAT(_RESULT__VAR, iii).status == RESULT_OK); else if (____CONCAT(_RESULT__VAR, iii).status == RESULT_NONE)
#define try_unwrap(dest, result) ____TRY_UNWRAP_IMPL(dest, result, __COUNTER__)

#define catch else

#ifdef __llvm__
#define lambda(ret, body) (void*)0
#else
#define lambda(ret, body) ({ ret __fn__ body __fn__;})
#endif