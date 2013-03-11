
/* Pyrite: a BASIC "compiler" implemented entirely using the C
 * macro system
 * 
 * All components are in the public domain
 * 
 * This file declares the few functions needed by the Pyrite runtime
 * (mainly GC and exceptions)
 */

#ifndef PYRITE_LIB_H
#define PYRITE_LIB_H


#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
/* */
#include "cmacros.h"

// TODO
#define THREADLOCAL

// Dummy GC markers for primitive types
#define _gc_Markint(x)
#define _gc_Markshort(x)
#define _gc_Markchar(x)
#define _gc_Markint64(x)
#define _gc_Markint32(x)
#define _gc_Markfloat(x)
#define _gc_Markdouble(x)
#define _gc_Markpointer(x)

#define _gc_MarkintArray _gc_Mark_primitiveArray
#define _gc_MarkshortArray _gc_Mark_primitiveArray
#define _gc_MarkcharArray _gc_Mark_primitiveArray
#define _gc_Markint64Array _gc_Mark_primitiveArray
#define _gc_Markint32Array _gc_Mark_primitiveArray
#define _gc_MarkfloatArray _gc_Mark_primitiveArray
#define _gc_MarkdoubleArray _gc_Mark_primitiveArray
#define _gc_MarkpointerArray _gc_Mark_primitiveArray

#define _gc_Markstring _gc_Mark_primitiveArray
#define _gc_MarkstringArray _gc_Mark_ObjectArray

// Dummy GC pushes
#define _gc_Pushint(x)
#define _gc_Pushshort(x)
#define _gc_Pushchar(x)
#define _gc_Pushint64(x)
#define _gc_Pushint32(x)
#define _gc_Pushfloat(x)
#define _gc_Pushdouble(x)
#define _gc_Pushpointer(x)

#define _gc_Pushvoid(x)

#define _gc_PushintArray(A) _gc_Push_Variable((_Object *)A)
#define _gc_PushshortArray(A) _gc_Push_Variable((_Object *)A)
#define _gc_PushcharArray(A) _gc_Push_Variable((_Object *)A)
#define _gc_Pushint64Array(A) _gc_Push_Variable((_Object *)A)
#define _gc_Pushint32Array(A) _gc_Push_Variable((_Object *)A)
#define _gc_PushfloatArray(A) _gc_Push_Variable((_Object *)A)
#define _gc_PushdoubleArray(A) _gc_Push_Variable((_Object *)A)
#define _gc_PushpointerArray(A) _gc_Push_Variable((_Object *)A)

#define _gc_Pushstring(A) _gc_Push_Variable((_Object *)A)
#define _gc_PushstringArray(A) _gc_Push_Variable((_Object *)A)

typedef void * pointer;
typedef int32_t int32;
typedef int64_t int64;

typedef struct objdata * objdataP;
typedef objdataP * _Object;

struct objdata {
	unsigned int d:1, l:1;
	char * ttag;
	void(*mark)(_Object);
	void(*del)(_Object);
	size_t sz;
	_Object nx;
};

typedef struct MetaArray { objdataP __meta; size_t len, es; void * data; } * MetaArray;
#define arraytype(T) typedef struct T##Array { struct MetaArray arr; T data[]; } * T##Array;
#define functype(N, T) typedef M_REST T(*N)M_FIRST T; arraytype(N) \
static inline void _gc_Push##N(void * _) {} \
static void(* const _gc_Mark##N##Array)(_Object) = _gc_Mark_primitiveArray; \
static inline void _gc_Push##N##Array(N##Array * a) {_gc_Push_Variable((_Object *)a);}

#define new(T) (T)GC_Allocate(sizeof(struct T), _gc_Mark##T, #T)
#define array(T, S) (T##Array)_gc_MakeArray(GC_Allocate(sizeof(struct T##Array) + (S) * sizeof(T), \
	_gc_Mark##T##Array, M_STR_(T##Array)), S, sizeof(T))
#define setfinaliser(OBJ) _gc_SetFinaliser(OBJ)
#define arraylen(A) ((A)->arr.len)

THREADLOCAL static MetaArray _ArrayTmp;
//#define elem(A, E) ((_ArrayTmp = (MetaArray)(A))[_arrayBoundsCheck(E)])
#define elem(A, E) (A)->data[E]

arraytype(int)
arraytype(short)
arraytype(char)
arraytype(int64)
arraytype(int32)
arraytype(float)
arraytype(double)
arraytype(pointer)

typedef charArray string;
arraytype(string)

struct _VarSlot {
	_Object * var;
	struct _VarSlot * nx;
};

enum _Frame_Type { FT_CALL_FRAME, FT_BLK_FRAME, FT_TRY_FRAME };
struct _Frame {
	enum _Frame_Type tag;
	struct _Frame * prev;
	struct _VarSlot * vars;
	jmp_buf * bufp;
};
extern struct _Frame _rootFrame;
extern struct _Frame * _topFrame;
extern _Object _currentException;

enum GC_Mode { GC_ON, GC_OFF };

void GC_Init(void);
void * GC_Allocate(size_t, void(*)(_Object), char *);
size_t GC_Collect(void);
void GC_SetMode(enum GC_Mode);
void GC_SetThreshold(size_t);

void _gc_FreeDefault(_Object);
void _gc_Mark_primitiveArray(_Object);
void _gc_Mark_ObjectArray(_Object);
void _gc_SetFinaliser(_Object, void(*)(_Object));
void _gc_LockObject(_Object, unsigned int);

void _gc_Push_Variable(_Object *);
void _gc_Push_Frame(struct _Frame *, enum _Frame_Type);
void _gc_Pop_Frame(enum _Frame_Type);

void ThrowException(_Object);
_Object GetException(void);

MetaArray _gc_MakeArray(_Object, size_t, size_t);
//int _arrayBoundsCheck(int);

string str(const char *);
int len(string);
string substr(string, unsigned int, unsigned int);
string join(string, string);
string upper(string);
string lower(string);
void print(string);
void write(string);


#endif

