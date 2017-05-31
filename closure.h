#ifndef CLOSURE_H
#define CLOSURE_H


#include "variadic_macros.h"


// XXX: It'd be really nice to have "real" closures in C.


#define ADD_SEMICOLON(x) x;


#define DECL_ARGS(...) FOR_EACH(ADD_SEMICOLON, __VA_ARGS__)


#define STRUCT_ARGS_NAME(fnname) fnname ## _args


#define DECL_STRUCT_ARGS(fnname, ...) struct fnname ## _args { DECL_ARGS(__VA_ARGS__) };


// XXX: C99 only
#define ARGS_LITERAL(...) { __VA_ARGS__ }
#define ARGS_COMPOUND_LITERAL(fnname, ...) ((fnname) { __VA_ARGS__ })
#define DEFINE_ARGS(fnname, sname, ...) struct STRUCT_ARGS_NAME(fnname) sname = ARGS_LITERAL(__VA_ARGS__)


#endif // CLOSURE_H
