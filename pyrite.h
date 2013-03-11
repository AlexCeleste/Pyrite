
/* Pyrite: a BASIC "compiler" implemented entirely using the C
 * macro system
 * 
 * All components are in the public domain
 * 
 * This file defines the BASIC syntax structures
 */

/* BASIC features provided:
 * 
 * + Function definition
 *   + Forward declarations
 * + Control blocks
 *   + While
 *   + For (upto, downto, to/step, in)
 *   + Repeat
 *   + Select
 *   + If
 *   + Try
 *   + Return
 * + Inline If
 * + Variable declarations
 *   + Let and var statements
 *   + With blocks
 *   + Mark variables
 * + Garbage collection
 *   + observe variables
 *   + allocate space
 *   + mark/sweep
 * + Infix text operators
 *   + andalso, orelse, not
 *   + b_and, b_or, xor
 *   + shr, shl (, sar), mod
 * + User-defined types
 * + Array type declaration
 * + String operations
 * - List operations
 */


#include "pyrite-lib.h"


// Necessary for paren-less block structures to work at the top level
#define BASIC(...) __VA_ARGS__
#define BASIC_0(...) __VA_ARGS__


// Function has two forms chosen by number of resolved args
#define function B_FUNCTION B_LEFTP
#define B_FUNCTION(...) M_CONC(B_FUNCTION_, M_NARGS(__VA_ARGS__))(__VA_ARGS__)

// 2: arguments and return type declared with C-like syntax
#define B_FUNCTION_2(AR, BODY) AR B_FBODY((void), M_ID BODY)
// 3: arguments declared with C-like syntax
#define B_FUNCTION_3(NA, R, BODY) R NA B_FBODY((void), M_ID BODY)
// 4: arguments and return type declared with BASIC syntax
#define B_FUNCTION_4(N, A, R, BODY) R N (B_ALIST A) B_FBODY(A, M_ID BODY)

#define B_REORD(n, t, ...) (__VA_ARGS__),t n
#define B_ALIST(...) M_REVERSE(M_RECUR(B_REORD, M_DIV2(M_NARGS(__VA_ARGS__)), (__VA_ARGS__)))
#define B_FBODY(A, B) { B_FPUSH A B B_FPOP(CALL) }

#define B_FPUSH(...) struct _Frame __fr; _gc_Push_Frame(&__fr, FT_CALL_FRAME); B_PUSHARGS(__VA_ARGS__)
#define B_PUSHARGS(...) M_FOR_EACH(B_FLD2, M_RECUR(B_PUSHARG, M_DIV2(M_NARGS(__VA_ARGS__)), (__VA_ARGS__)))
#define B_PUSHARG(n, t, ...) (__VA_ARGS__), B_PUSHVAR(n, t)
#define B_FPOP(T) _gc_Pop_Frame(FT_##T##_FRAME);

#define declare(...) M_FOR_EACH(M_ID, M_RECUR(B_DECL, M_DIV3(M_NARGS(__VA_ARGS__)), (__VA_ARGS__)))
#define B_DECL(n, a, r, ...) (__VA_ARGS__), r n a;


#define type B_TYPE B_LEFTP(
#define B_TYPE(N, ...)  typedef struct M_ID N * M_ID N; \
	struct M_ID N { objdataP __meta; M_FOR_EACH(B_FLD2, \
	M_FOR_EACH(B_FIELD, __VA_ARGS__)) }; arraytype N \
	B_GENGCFUNC(M_ID N, __VA_ARGS__)
#define B_FIELD(F) B_ALIST F;
#define B_FLD2(F) F;

#define field ),(
#define endtype ))

#define B_GENGCFUNC(N, ...) static void M_CONC(_gc_Mark, N)(_Object o) { \
N obj = (N)o; BASIC_0(if not (obj->__meta->d) do \
M_FOR_EACH(M_ID, M_FOR_EACH(B_MARKFLD, __VA_ARGS__)) obj->__meta->d = 1 end) } \
static inline void M_CONC(_gc_Push, N)(N * v) {_gc_Push_Variable((_Object *)v);} \
static void(* const M_CONC(_gc_Mark, M_CONC(N, Array)))(_Object) = _gc_Mark_ObjectArray; \
static inline void M_CONC(_gc_Push, M_CONC(N, Array))(M_CONC(N, Array) * a) \
{_gc_Push_Variable((_Object *)a);}
#define B_MARKFLD(...) M_RECUR(B_MARKF2, M_DIV2(M_NARGS __VA_ARGS__), __VA_ARGS__)
#define B_MARKF2(n, t, ...) (__VA_ARGS__), M_CONC(_gc_Mark, t)((_Object)obj->n);


#define let ;B_LET B_LEFTP(
#define B_LET(V) M_FOR_EACH(B_FLD2, B_LETLIST V)
#define B_LETLIST(...) M_REVERSE(M_RECUR(B_RELET, M_DIV3(M_NARGS(__VA_ARGS__)), (__VA_ARGS__)))
#define B_RELET(n, t, v, ...) (__VA_ARGS__), t n = v; B_PUSHVAR(n, t)

#define var(...) ;M_FOR_EACH(B_FLD2, B_ALIST(__VA_ARGS__))

#define with B_WITH B_LEFTP
#define B_WITH(...) { B_WITHV(M_HEAD(__VA_ARGS__)) M_ID2(M_TAIL(__VA_ARGS__)) B_FPOP(BLK) }
#define B_WITHV(...) struct _Frame __fr; _gc_Push_Frame(&__fr, FT_BLK_FRAME); \
M_FOR_EACH(B_FLD2, M_REVERSE(M_RECUR(B_REWITH, M_DIV2(M_NARGS(__VA_ARGS__)), (__VA_ARGS__))))
#define B_REWITH(n, t, ...) (__VA_ARGS__), t n; B_PUSHVAR(n, t)

// Primitive typed vars will get dummied out
#define B_PUSHVAR(n, t) _gc_Push##t(&n)
// Only track variables of non-primitive type!
#define gc_trackvar(n) _gc_PushVariable((_Object *)n)


#define while ;while B_WHILE B_LEFTP
#define B_WHILE(P, CODE) ( P ) { M_ID CODE }

// For loop has several forms, chosen by number of resolved args
#define for ;for B_FOR B_LEFTP
#define B_FOR(...) M_CONC(B_FOR_, M_NARGS(__VA_ARGS__))(__VA_ARGS__)

// 6: For x = a To b Step c
#define B_FOR_6(V, F, T, _, S, CODE) (V = F; V <> T; V += S) { M_ID CODE }
// 5: For x = a upto/downto b
#define B_FOR_5(V, S, OP, E, CODE) (V = S; OP##2(V, E); OP##1(V)) { M_ID CODE }
// 4: For x = a To b
#define B_FOR_4(V, F, T, CODE) (V = F; V <= T; V++) { M_ID CODE }
// 3: For elem = EachIn List
#define B_FOR_3(V, LIST, CODE) (B_ITERATOR B_INAME = B_GETITERATOR(LIST); \
B_HASNEXT(B_INAME);) { V = B_NEXT(B_INAME); { M_ID CODE } }

#define upto ,B_UPTO_,
#define downto ,B_DOWNTO_,
#define B_UPTO_2(V, T) V < T
#define B_UPTO_1(V) V++
#define B_DOWNTO_2(V, T) V > T
#define B_DOWNTO_1(V) V--


// If chooses between inline and block by number of resolved args
#define if ;if B_IF B_LEFTP
#define B_IF(...) M_CONC(B_IF_, M_NARGS(__VA_ARGS__))(__VA_ARGS__)

// 1: If/Then
#define B_IF_1(P) (P)
// 2: If/Do
#define B_IF_2(P, CODE) (P) { M_ID CODE }
// 3: If/Do/ElseDo
#define B_IF_3(P, CODET, CODEE) (P) { M_ID CODET } else { M_ID CODEE }

#define then )
#define elseif end else if
#define elsedo ;),(


#define select ;B_SELECT B_LEFTP ((
#define B_SELECT(VAL, ...) { B_SELECTOR B_SELNAME = M_ID(M_FIRST M_ID VAL); \
M_FOR_EACH(B_CASE1, __VA_ARGS__) {} }
#define B_CASE(...) (M_FOR_EACH(B_CASE0, M_HEAD(__VA_ARGS__)) 0, M_TAIL(__VA_ARGS__))
#define B_CASE0(X) X == B_SELNAME ||
#define B_CASE1(C) BASIC_0(if M_FIRST(M_ID C) then {M_ID M_REST(M_ID C);} else)

#define case )),B_CASE B_LEFTP
#define default )),B_CASE B_LEFTP B_SELNAME,()


#define repeat ;{ _B_REPEAT: {
#define until(P) ;} if !(P) then goto _B_REPEAT; }
#define forever ;} goto _B_REPEAT; }


#define try ;B_TRY B_LEFTP(
#define B_TRY(...) { jmp_buf __buf; struct _Frame __fr; _gc_Push_Frame(&__fr, FT_TRY_FRAME); __fr.bufp = &__buf; \
BASIC_0(if !setjmp(__buf) then { M_ID M_FIRST(__VA_ARGS__ ); } else { \
M_FOR_EACH(B_CATCH, M_REST(__VA_ARGS__)) {B_FPOP(TRY) throw(exception)} }) B_FPOP(TRY) }
#define B_CATCH(B) B_CAT(M_FIRST B, M_REST B)
#define B_CAT(T, C) BASIC_0(if !strcmp(M_STR_(T), (*exception)->ttag) then { C; } else)

#define throw(E) ThrowException((_Object)E);
#define catch(E) ),(E,
#define exception GetException()


#define return ;{ return B_RET B_LEFTP(
#define B_RET(R) _gc_Pop_Frame(FT_CALL_FRAME), M_ID R }


#define and &&
#define or ||
#define not !
#define bitand &
#define bitor |
#define xor ^
#define shr >>
#define shl <<
#define mod %


#define B_LEFTP (
#define B_RIGHTP )
#define as ,
#define of ,
#define do ,(
#define end ;))
#define in ,
#define to ,
#define step ,,
#define equal ,
#define null NULL
#define none ,void

#define B_ITERATOR Iterator
#define B_INAME _i
#define B_GETITERATOR getIterator
#define B_HASNEXT hasNext
#define B_NEXT next

#define B_SELECTOR int
#define B_SELNAME _val

//BASIC(
/*
function void f2(int argc, char * argv) do
	printf("Hello world!\n")
end
*/
/*
function f3 as (argc as int, argv as stringArray) of void do
	printf("Hello world!\n")
end
*/
/*
functype(IntF, ((int), void) )
function f4 as (x as int, f as IntF) of void do
	f(x)
end
*/
/*
function f5 as (none) of void do
	print("hello\n")
end
*/
/*
declare(f6 as (IntF, int) of void,
		f7 as (Vector3) of int)
*/

/*
type Vector3
	field x as float, y as float
	field z as float
endtype
*/

/*
while x == 1 do
	print(x)
end
*/

/*
for a in 0 upto 10 do
	DoSomething(a)
end
*/
/*
for e in list do
	Something(e)
end
*/
/*
for x in a to b do
	print(x)
end
*/

//if a then b else c
/*
if a do
	b
elseif c do
	d
elsedo
	e
end
*/

/*
select x in
	case a, b do
		print(a)
	case c, d do
		print(d)
	default
		print(0)
end
*/

/*
repeat
	print(a)
//forever
until(a == 2)
*/

/*
try
	something()
catch(Vector3)
	handle(exception)
catch(string)
	handle2(exception)
end
*/

//return foo end

/*
let
	i as int equal 10, j as int equal 20
end
var(a as int, b as float)
*/
/*
with a as int, s as string do
	write(s, a)
end
*/
//)

