
// Pyrite BASIC compiler test

#include <stdio.h>
#include "pyrite.h"

void func(int a) { printf("func: %d\n", a); }

BASIC(

type Vec3
	field x as float, y as float
	field z as float
endtype

declare(bar as () of void,
		baz as (int) of int)

functype(IntF, ((int), void))

function foo as (i as int, j as int) of int do
	let
		o as Vec3 equal new(Vec3), a as stringArray equal array(string, 3),
		s as string equal str("123"),
		c as charArray equal array(char, 5),
		f as IntF equal func,
		fa as IntFArray equal array(IntF, 5)
	end
	elem(a, 0) = s ; elem(a, 1) = str("_456_") ; elem(a, 2) = str("789");
	print(join(str("ABC"), str("DEF"))); bar()
	
	var(x as int)	// 'var' doesn't link variables to the GC system
	for x in 0 to 2 do
		print(elem(a, x))
	end
	
	return 2 * (i + j) end
end

type myEx
	field code as int, msg as string
endtype

function handle as (e as myEx) of void do
	printf("Handled exception with code %d and message %s\n",
			e->code, e->msg->data)
end

function bar as (none) of void do
	try
		printf("Trying baz...\n");
		baz(45);
		printf("Tried baz!\n")
	catch(myEx)
		printf("Caught a myEx!\n"); handle((myEx)exception)
	end
end

function baz as (x as int) of int do
	printf("baz got: %d\n", x)
	let e as myEx equal new(myEx) end
	e->code = 67 ; e->msg = str("'a message'") ; throw(e)
	return x + 1 end
end

function printSA as (a as stringArray) of void do
	print(str("Printing string array:"))
	var(i as int)
	for i in 0 upto arraylen(a) do
		print(join(str("  "), elem(a, i)))
	end
end

)

// Undefine BASIC keywords so C code will work
#include "pyrite-undefs.h"

int main(int argc, char ** argv) {
	GC_Init();
	GC_SetMode(GC_OFF);
	
	stringArray avec = array(string, argc);
	for (int i = 0; i < argc; i++) {
		elem(avec, i) = str(argv[i]);
	}
	
	GC_SetMode(GC_ON);
	
	int r = foo(42, 47);
	printf("foo: %d\n", r);
	printSA(avec);
	
	r = GC_Collect();
	printf("Freed %d bytes\n", r);
	printf("All works fine..?\n");
	
	return 0;
}

