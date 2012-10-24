/*
 * syscall.c
 *
 *  Created on: Sep 8, 2012
 *      Author: bradley
 */

#include <geekos/errno.h>
#include <conio.h>
#include <process.h>

int main(int argc, char **argv) {

	int maxLimit = atoi(argv[1]); /* Max number of system calls */
	int numNull = atoi(argv[2]); /* Number of null system calls */
	int i;

	Limit(0, maxLimit); /* Limit system call */

	for (i = 0; i < numNull; i++){
		//Print("null %d\n", i);
		Null();
	}

	return 0;
}
