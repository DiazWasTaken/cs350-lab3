//Brian Cabrera Diaz
/*


NOTES FOR HELP
validation is 247-288


help with validate I dont understand how to make that checksum thing






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

#include "viktar.h"

#define BUFFER_SIZE 10

static int isVerbose = TRUE;


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

	//unsigned char buffer[BUFFER_SIZE] = {'\0'};

	char formatted_time[256];  // Buffer to hold the formatted date and time
	time_t mod_time;
	time_t acc_time;
	struct tm *timeinfo;

	MD5_CTX header_ctx, data_ctx;
	off_t remaining_size;
	uint8_t digest[MD5_DIGEST_LENGTH];
	ssize_t bytes_read = 0;
	MD5_CTX context;
	

	//parsing command line segment
	{
		int opt = 0;
		while((opt = getopt(argc, argv, OPTIONS)) != -1) {
			switch (opt) {
			case 'x':// Extract members
				break;
			case 'c':// create a viktar style archive file
				break;
			case 't':// Short table of contents
				break;
			case 'T':// Long table of contents
				tocChoice++;
				break;
			case 'f':// specify the name of viktar file
				viktarFile = optarg;
				//printf("File name passed in: %s\n", viktarFile);
				break;
			case 'V':// validate content of archive member
				validate++;
				break;
			case 'h':// show help text and exit | matches prof in file but not terminal 10/28/2024
				printf(" help text\n");
				printf("	./viktar\n");
				printf("	Options: xctTf:Vhv\n");
				printf("		-x              extract file/files from archive\n");
				printf("		-c              create an archive file\n");
				printf("		-t              display a short table of contents of the archive file\n");
				printf("		-T              display a long table of contents of the archive file\n");
				printf("		Only one of xctTV can be specified\n");
				printf("		-f filename use filename as the archive file\n");
				printf("		-V		validate the MD5 values in the viktar file\n");
				printf("		-v		give verbose diagnostic messages\n");
				printf("		-h		display this AMAZING help message\n");
				exit(EXIT_SUCCESS);	
				break;
			case 'v':// verbose processing
				//printf("verbose is now enabled\n");
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
		//printf("Here are the files to work for a viktar file\n");
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
		//printf("Entered archive file name: %s\n", viktarFile);
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
			
			printf("\t\tmode:           ");
			printf((fileMode & S_IRUSR) ? "-r" : "-");
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
				printf("\t\tgroup:      %s\n", grp->gr_name);
			
			printf("\t\tsize:           %ld\n", md.st_size);
			
			//mod_time = md.st_mtime.tv_sec;
			mod_time = md.st_mtim.tv_sec;
			timeinfo = localtime((&mod_time));
			strftime(formatted_time, 256, "%Y-%m-%d %H:%M:%S %Z", timeinfo);
			printf("\t\tmtime:          %s\n", formatted_time);

			acc_time = md.st_atim.tv_sec;
			timeinfo = localtime((&acc_time));
			strftime(formatted_time, 256, "%Y-%m-%d %H:%M:%S %Z", timeinfo);
			printf("\t\tatime:          %s\n", formatted_time);	

			lseek(iarch, md.st_size, SEEK_CUR);
			
			read(iarch, &footer, sizeof(viktar_footer_t));
			   // Print md5sum_header
    			printf("\t\tmd5 sum header: ");
    			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        			printf("%02x", footer.md5sum_header[i]);
    			}
    			printf("\n");

    			// Print md5sum_data
    			printf("\t\tmd5 sum data:   ");
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
//small and large toc is working as of 11/4/2024



//this section of code will validate the md5 data.
if (validate > 0) {
    // Reopen the file if not already open
    if (viktarFile != NULL) {
        iarch = open(viktarFile, O_RDONLY);
        if (iarch < 0) {
            perror("Error opening viktar file");
            exit(EXIT_FAILURE);
        }
    }

    // Validate the tag at the beginning of the file
    read(iarch, buf, strlen(VIKTAR_TAG));
    if (strncmp(buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0) {
        fprintf(stderr, "This is not a valid viktar file\n");
        exit(EXIT_FAILURE);
    }

    // MD5 checksum validation process
    while (read(iarch, &md, sizeof(viktar_header_t)) > 0) {
        // Initialize MD5 contexts for header and data
        MD5Init(&header_ctx);
        MD5Init(&data_ctx);

        // Calculate MD5 for header
        MD5Update(&header_ctx, (void *)&md, sizeof(viktar_header_t));// always pass in with this (void *) type whenever you pass in &md NOTE**********
        MD5Final(digest, &header_ctx);

		//comparing header.st_size with the actual file bytes read from iarch
        // Read footer and compare header checksum
        read(iarch, &footer, sizeof(viktar_footer_t));
        if (memcmp(digest, footer.md5sum_header, MD5_DIGEST_LENGTH) != 0) {
            fprintf(stderr, "Header checksum mismatch for file %s\n", md.viktar_name);
            continue; // Skip to the next file if header checksum fails
        }

        // Calculate MD5 for file data
		remaining_size = md.st_size;
        while (remaining_size > 0) {
            ssize_t chunk_size = (remaining_size < BUFFER_SIZE) ? remaining_size : BUFFER_SIZE;
            bytes_read = read(iarch, buffer, chunk_size);
            if (bytes_read <= 0) break;

            MD5Update(&data_ctx, buffer, bytes_read);
            remaining_size -= bytes_read;
        }
        MD5Final(digest, &data_ctx);

        // Compare data checksum
        if (memcmp(digest, footer.md5sum_data, MD5_DIGEST_LENGTH) != 0) {
            fprintf(stderr, "Data checksum mismatch for file %s\n", md.viktar_name);
        } else {
            printf("File %s passed validation\n", md.viktar_name);
        }
    }

    if (viktarFile != NULL) {
        close(iarch);
    }
}


return EXIT_SUCCESS;
}

