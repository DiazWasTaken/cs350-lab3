//Brian Cabrera Diaz
/*
10/28/2024 
today my goal is just makefile and make the arg line thing and maybe make it into a function to keep ready for all future assignments and such
NOTE**** 
10/29/2024
today the goal in code party is to get at least the short table of contents done
NOTE**** we have completed the getopt input of filename
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <md5.h>
#include <fcntl.h>
#include <string.h>

#include "viktar.h"

static int isVerbose = FALSE;

//prototypes
void shortTable(char *file);

int main(int argc, char *argv[])
{
//variables 
	char *viktarFile = NULL;
	int iarch = STDIN_FILENO;
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
				viktarFile = optarg;
				printf("File name passed in: %s\n", viktarFile);
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
// at this point getopt is done processing and there should be only what files I want to work on left on the command line
// this section of code needs to worry about picking up what files to place within the archive
	if (optind < argc){
		fprintf(stderr,"\nThis is what remains on the command line after getopt:\n");
		for (int j = optind; j < argc; j++){
			fprintf(stderr, "\t%s\n", argv[j]);
		}
	}
// here will be a verbose statement to check for each file that was passed in
	if(isVerbose){
		printf("Here are the files to work for a viktar file\n");
	}
// this section will worry about the actual name of the archive file
	if (viktarFile == NULL){
		printf("Please enter what archive file you want to operate on/create:\n");
		//viktarFile = sscanf("%s");
	}
	else{
		//archiveFD = open(fileName, O_RDONLY);
	}
// second verbose to print what archive name was passed in
	if(isVerbose){
		printf("Entered archive file name: %s\n", viktarFile);
	}

/*
at this time I want to make that small toc section
all of this is yoinked from the class slides for viktar
*/
	// opening the file and setting that file descriptor to iarch
	if (viktarFile != NULL){
		iarch = open(viktarFile, O_RDONLY);
	}

	//char buf[100] = {\0};// it doesnt like this for some reason
	char buf[100];
	
	//now we validate the tag
	read(iarch, buf, strlen(VIKTAR_TAG));

	if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
		//this isnt a valid viktar file
		//print a message making fun of them and dip
		fprintf(stderr, "lmfao gitgud\n");
		exit(EXIT_FAILURE);
	}
	
	viktar_header_t md;// this is a variable for the viktar header meta data
	//time to process said meta data
	printf("Contents of viktar file: \"%s\"\n", viktarFile != NULL ? viktarFile: "stdin");
	
	while (read(iarch, &md, sizeof(viktar_header_t)) > 0){
		memset(buf, 0, 100);
		strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
		printf("\tfile name: %s\n", buf);
		lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
	}
	//we have now finished processing the archive members in the file 
	// now the read should return 0 meaning we hit EOF (end of file)
	if (viktarFile != NULL) { 
		close(iarch);
	}		
//small toc is working as of 10/29/2024

/*
now we are in spooky territory im gonna be adlibbing this hopefully nothing dies but ill have a git push in case it does
*/

return EXIT_SUCCESS;
}


void shortTable(char *file)
{
}


















