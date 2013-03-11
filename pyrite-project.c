
/* Pyrite: a BASIC "compiler" implemented entirely using the C
 * macro system
 * 
 * All components are in the public domain
 * 
 * This file defines a minimal wrapper file so that programs may appear
 * to have been written entirely in Pyrite code.
 * 
 * To compile a Pyrite-project program with GCC, try:
 *   gcc -std=c99 pyrite-lib.c -DPYRITE_PROJECT=\"myPyriteFile.pyr\"
 *     pyrite-project.c -o myAppName
 */

#include <stdio.h>
#include "pyrite.h"


#include PYRITE_PROJECT


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
	
	int r = MAIN(avec);
	return r;
}

