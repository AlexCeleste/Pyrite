
/* Pyrite: a BASIC "compiler" implemented entirely using the C
 * macro system
 * 
 * All components are in the public domain
 * 
 * This file defines Pyrite's runtime library: GC, exceptions, and a
 * few collection and string functions
 */

#include <signal.h>
#include <stdio.h>
#include "pyrite-lib.h"

struct _Frame _rootFrame;
struct _Frame * _topFrame;
_Object _currentException;

// Maximum number of variable slots on the stack;
// probably should be dynamic anyway...
#define MAX_SLOTS 200000
static struct _VarSlot slot[MAX_SLOTS];	// Shadow stack for tracing
static struct _VarSlot * topslot;
static _Object last;	// List of allocated objects
static enum GC_Mode gc_mode;
static size_t bytes_allocated;
static size_t threshold;
#define DEFAULT_THRESHOLD 1000000

// Preallocated exception message for out-of-memory
static struct panic_ex {
	objdataP __meta;
	char msg[1024];
	struct objdata data;
} panic = { .msg = "GC ran out of memory trying to allocate object",
			.data = { .l = 1, .ttag = "GCException" }
		};


// GC system functions

void GC_Init(void) {
	bytes_allocated = 0;
	for (int i = 0; i < MAX_SLOTS; i++) {	// Set up the varslots array
		slot[i].nx = &slot[i] + 1;
	}
	slot[MAX_SLOTS - 1].nx = NULL;
	topslot = &slot[0];
	_topFrame = NULL;
	threshold = DEFAULT_THRESHOLD;	// Apply default GC settings (aggressiveness?)
	gc_mode = GC_ON;
}

void * GC_Allocate(size_t s, void(*mark)(_Object), char * ttag) {
	if (bytes_allocated >= threshold) {	// Do a collection if necessary
		GC_Collect();
	}
	size_t sz = s + sizeof(struct objdata);
	_Object o = calloc(1, sz);
	if (o == NULL) {
		ThrowException(&panic.__meta);	// Panic, out of memory
	}
	*o = (objdataP)(((char *)o) + s);
	**o = (struct objdata){
		.ttag = ttag, .mark = mark, .del = _gc_FreeDefault, .sz = sz
	};
	(*o)->nx = last;	// Add object to all-objects list
	last = o;
	bytes_allocated += sz;	// Update GC status
//	printf("Allocated %d bytes as %s at %p\n", sz, ttag, o);
	return o;
}

size_t GC_Collect(void) {
	size_t bytes = 0;
	if (gc_mode == GC_ON) {
		for (_Object o = last; o != NULL; o = (*o)->nx) {
			(*o)->d = (*o)->l;
		}
		for (struct _Frame * f = _topFrame; f != NULL; f = f->prev) {
			for (struct _VarSlot * v = _topFrame->vars; v != NULL; v = v->nx) {
				_Object o = *v->var;
				(*o)->mark(o);
			}
		}
		for (_Object o = last, n, *dest = &last; o != NULL; o = n) {
			n = (*o)->nx;
			if (!(*o)->d) { 
				*dest = n;
				bytes += (*o)->sz; (*o)->del(o);
			} else {
				dest = &((*o)->nx);
			}
		}
		bytes_allocated -= bytes;
	}
	return bytes;
}

void GC_SetMode(enum GC_Mode mode) {
	gc_mode = mode;
}

void GC_SetThreshold(size_t bytes) {
	threshold = bytes;
}


// Per-object GC methods

void _gc_FreeDefault(_Object o) {
	free(o);
}

void _gc_Mark_primitiveArray(_Object o) {
	MetaArray a = (MetaArray)o;
	a->__meta->d = 1;
}

void _gc_Mark_ObjectArray(_Object o) {
	MetaArray a = (MetaArray)o;
	if (!a->__meta->d) {
		a->__meta->d = 1;
		for (int i = 0; i < a->len; i++) {
			o = ((_Object *)(a->data))[i];
			(*o)->mark(o);
		}
	}
}

void _gc_SetFinaliser(_Object o, void(*del)(_Object)) {
	(*o)->del = del;
}

void _gc_LockObject(_Object o, unsigned int l) {
	(*o)->l = !!l;
}
int printf(const char *, ...);

// Shadow stack management

void _gc_Push_Variable(_Object * v) {
	struct _VarSlot * s = topslot;
	if (s == NULL) {
		fprintf(stderr, "Shadow stack overflow\n");
		raise(SIGSEGV);	// Panic! Shadow stack overflow
	}
	topslot = topslot->nx;
	s->var = v;
	s->nx = _topFrame->vars;
	_topFrame->vars = s;
}

static void _gc_Pop_Variable(void) {
	struct _VarSlot * s = _topFrame->vars;
	_topFrame->vars = s->nx;
	s->nx = topslot;
	topslot = s;
}

void _gc_Push_Frame(struct _Frame * f, enum _Frame_Type t) {
	f->prev = _topFrame;
	f->tag = t;
	f->vars = NULL;
	_topFrame = f;
}

void _gc_Pop_Frame(enum _Frame_Type t) {
	struct _Frame * f;
	do {
		while (_topFrame->vars != NULL) {
			_gc_Pop_Variable();
		}
		f = _topFrame;
		_topFrame = _topFrame->prev;
	} while (t == FT_CALL_FRAME && f->tag != FT_CALL_FRAME);
}


// Exceptions

void ThrowException(_Object ex) {
	while (_topFrame->tag != FT_TRY_FRAME) {
		_gc_Pop_Frame(FT_TRY_FRAME);
	}
	if (_topFrame == NULL) {
		fprintf(stderr, "Unhandled exception %p\n", ex);
		raise(SIGSEGV);	// Seems a bit redundant really...
	}
	_currentException = ex;
	longjmp(*(_topFrame->bufp), 1);
}

_Object GetException(void) {
	return _currentException;
}


// Array access

MetaArray _gc_MakeArray(_Object o, size_t len, size_t es) {
	MetaArray a = (MetaArray)o;
	a->len = len; a->es = es; a->data = ((char *)o) + sizeof(struct MetaArray);
	return a;
}

/*int _arrayBoundsCheck(int) {
	
}*/


// Strings

string str(const char * c) {
	string s = array(char, (strlen(c) + 1));
	strcpy(&(s->data[0]), c);
	return s;
}

int len(string s) {
	return s->arr.len - 1;
}

string substr(string s, unsigned int from, unsigned int len) {
	if (from >= s->arr.len) { return array(char, 1); }
	if (from + len > s->arr.len) { len -= (from + len) - s->arr.len; }
	string r = array(char, (len + 1));
	memcpy(r->data, &s->data[from], len);
	return r;
}

string join(string a, string b) {
	string r = array(char, (len(a) + len(b)));
	memcpy(r->data, a->data, len(a));
	memcpy(&r->data[len(a)], b->data, len(b));
	return r;
}

string upper(string a) {
	string r = array(char, len(a));
	memcpy(r->data, a->data, len(a));
	for (int i = 0; i < len(r); i++) {
		if (r->data[i] >= 97 && r->data[i] <= 122 ) { r->data[i] -= 32; }
	}
	return r;
}

string lower(string a) {
	string r = array(char, len(a));
	memcpy(r->data, a->data, len(a));
	for (int i = 0; i < len(r); i++) {
		if (r->data[i] >= 65 && r->data[i] <= 90 ) { r->data[i] += 32; }
	}
	return r;
}

void print(string s) {
	printf("%s\n", s->data);
}

void write(string s) {
	printf("%s", s->data);
}

