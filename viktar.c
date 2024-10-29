//Brian Cabrera Diaz
/*
10/28/2024 
today my goal is just makefile and make the arg line thing and maybe make it into a function to keep ready for all future assignments and such
NOTE**** 
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <md5.h>

#include "viktar.h"

static int isVerbose = FALSE;


int main(int argc, char *argv[])
{
	//parsing command line segment
	{
		int opt = 0;
		while((opt = getopt(argc, argv, OPTIONS)) != -1) {
			switch (opt) {
			case 'x':// Extract members
				printf("showing case x\n");
				break;
			case 'c':// create a viktar style archive file
				printf("showing case c\n");
				break;
			case 't':// Short table of contents
				printf("showing case t\n");
				break;
			case 'T':// Long table of contents
				printf("showing case T\n");
				break;
			case 'f':// specify the name of viktar file
				printf("showing case f\n");
				break;
			case 'V':// validate content of archive member
				printf("showing case V\n");
				break;
			case 'h':// show help text and exit | matches prof in file but not terminal 10/28/2024
				printf("Showing help text\n");
				printf("	./viktar\n");
				printf("	Options: xctTf:Vhv\n");
				printf("		-x		Extract files/files from archive\n");
				printf("		-c      Create an archive file\n");
				printf("		-t		display a short table of contents of the archive file\n");
				printf("		-T		display a long table of contents of the arhive file\n");
				printf("		Only one of xctTV can be specified\n");
				printf("		-f filename use filename as the archive file\n");
				printf("		-V		validate the MD5 values in the viktar file\n");
				printf("		-v		give verbose diagnostic messages\n");
				printf("		-h		display this super help message\n");
				
				break;
			case 'v':// verbose processing
				printf("verbose is now enabled\n");
				isVerbose = !isVerbose;
				break;
			default: 
				printf("this is default\n");
				break;

			}
		}
	}




return EXIT_SUCCESS;
}
