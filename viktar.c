//Brian Cabrera Diaz
/*
10/28/2024 
today my goal is just makefile and make the arg line thing and maybe make it into a function to keep ready for all future assignments and such
NOTE**** 
10/29/2024
today the goal in code party is to get at least the short table of contents done
NOTE**** we have completed the getopt input of filename
10/30/24
today we want to finish long table of contents
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <md5.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
//#include <bits/getopt_core.h>

#include "viktar.h"


static int isVerbose = FALSE;


int main(int argc, char *argv[])
{
//variables 
	//these variables are for making choices with the swtich cases
	int tocChoice = 0;
	int validate = 0;

	//these variables are for the actual variables the program needs
	char *viktarFile = NULL;
	int iarch = STDIN_FILENO;
	int fileMode;
	struct passwd *pwd;
	struct group *grp;	
	viktar_header_t md;// this is a variable for the viktar header meta data
	viktar_footer_t footer; 
	char buf[100] = "\0";// fixed now | each spot is now null
	//char buf[100];
	

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
				tocChoice++;
				break;
			case 'f':// specify the name of viktar file
				printf("showing case f\n");
				viktarFile = optarg;
				printf("File name passed in: %s\n", viktarFile);
				break;
			case 'V':// validate content of archive member
				printf("showing case V\n");
				validate++;
				break;
			case 'h':// show help text and exit | matches prof in file but not terminal 10/28/2024
				printf("Showing help text\n");
				printf("	./viktar\n");
				printf("	Options: xctTf:Vhv\n");
				printf("		-x		Extract files/files from archive\n");
				printf("		-c              Create an archive file\n");
				printf("		-t		display a short table of contents of the archive file\n");
				printf("		-T		display a long table of contents of the arhive file\n");
				printf("		Only one of xctTV can be specified\n");
				printf("		-f filename use filename as the archive file\n");
				printf("		-V		validate the MD5 values in the viktar file\n");
				printf("		-v		give verbose diagnostic messages\n");
				printf("		-h		display this super help message\n");
				exit(EXIT_SUCCESS);	
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
for both tocs **** im gonna be adding a toc vairable that is either TRUE for small and false for large or other way around
*/
	// opening the file and setting that file descriptor to iarch
	if (viktarFile != NULL){
		iarch = open(viktarFile, O_RDONLY);
	}

	
	
	//now we validate the tag
	read(iarch, buf, strlen(VIKTAR_TAG));

	if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
		//this isnt a valid viktar file
		//print a message making fun of them and dip
		fprintf(stderr, "This is not a valid viktar file\n");
		exit(EXIT_FAILURE);
	}
	
	//time to process said meta data
	printf("Contents of viktar file: \"%s\"\n", viktarFile != NULL ? viktarFile: "stdin");

	if (tocChoice == 0){	
		while (read(iarch, &md, sizeof(viktar_header_t)) > 0){
			memset(buf, 0, 100);
			strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
			printf("\tfile name: %s\n", buf);
			lseek(iarch, md.st_size + sizeof(viktar_footer_t), SEEK_CUR);
		}
	}
	else{// now we are in spooky territory im gonna be adlibbing this hopefully nothing dies but ill have a git push in case it does
		while (read(iarch, &md, sizeof(viktar_header_t)) > 0){
			memset(buf, 0, 100);
			strncpy(buf, md.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
			printf("\tfile name: %s\n", buf);

			fileMode = md.st_mode;
			
			printf("\t\tmode:    ");
			printf((fileMode & S_IRUSR) ? "r" : "-");
    			printf((fileMode & S_IWUSR) ? "w" : "-");
    			printf((fileMode & S_IXUSR) ? "x" : "-");
    			printf((fileMode & S_IRGRP) ? "r" : "-");
    			printf((fileMode & S_IWGRP) ? "w" : "-");
    			printf((fileMode & S_IXGRP) ? "x" : "-");
    			printf((fileMode & S_IROTH) ? "r" : "-");
    			printf((fileMode & S_IWOTH) ? "w" : "-");
    			printf((fileMode & S_IXOTH) ? "x" : "-");
			printf("\n");

			if ((pwd = getpwuid(md.st_uid)) != NULL)
				printf("\t\tuser:       %s\n", pwd->pw_name);
			if ((grp = getgrgid(md.st_gid)) != NULL)
				printf("\t\tgroup: %s\n", grp->gr_name);
			
			printf("\t\tSize:    %ld\n", md.st_size);

			printf("\t\tmtime: %s", ctime(&md.st_mtime));
			printf("\t\tatime: %s", ctime(&md.st_atime));	

			lseek(iarch, md.st_size, SEEK_CUR);
			
			read(iarch, &footer, sizeof(viktar_footer_t));
			   // Print md5sum_header
    			printf("MD5 Header: ");
    			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        			printf("%02x", footer.md5sum_header[i]);
    			}
    			printf("\n");

    // Print md5sum_data
    			printf("MD5 Data: ");
    			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        			printf("%02x", footer.md5sum_data[i]);
    			}
    			printf("\n");
	
		}
		
		
	}
	//we have now finished processing the archive members in the file 
	// now the read should return 0 meaning we hit EOF (end of file)
	if (viktarFile != NULL) { 
		close(iarch);
	}		
//small toc is working as of 10/29/2024



//this section of code will validate the md5 data.
if(validate > 0){


}





return EXIT_SUCCESS;
}

