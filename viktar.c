#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include "md5.h"

#include "viktar.h"

// Function Prototypes
void print_help(void);
void handle_verbose(int verbose, const char *message);
int open_archive(const char *filename, int flags, mode_t mode);
void create_archive(const char *archive_filename, char **members, int count, int verbose);
void extract_archive(const char *archive_filename, char **members, int count, int verbose);
void short_toc(const char *archive_filename, int verbose);
void long_toc(const char *archive_filename, int verbose);
void validate_archive(const char *archive_filename, int verbose);

// Implementation of print_help
void print_help(void)
{
    printf("help text\n");
    printf("\tOptions: xctTf:Vhv\n");
    printf("\t\t-x\t\textract file/files from archive\n");
    printf("\t\t-c\t\tcreate an archive file\n");
    printf("\t\t-t\t\tdisplay a short table of contents of the archive file\n");
    printf("\t\t-T\t\tdisplay a long table of contents of the archive file\n");
    printf("\t\tOnly one of xctTV can be specified\n");
    printf("\t\t-f filename\tuse filename as the archive file\n");
    printf("\t\t-v\t\tgive verbose diagnostic messages\n");
    printf("\t\t-h\t\tdisplay this AMAZING help message\n");
}

// Implementation of handle_verbose
void handle_verbose(int verbose, const char *message)
{
    if (verbose)
    {
        fprintf(stderr, "%s\n", message);
    }
}

// Implementation of open_archive
int open_archive(const char *filename, int flags, mode_t mode)
{
    if (filename)
    {
        int fd = open(filename, flags, mode);
        if (fd == -1)
        {
            perror("Error opening archive file");
            exit(EXIT_FAILURE);
        }
        return fd;
    }
    else
    {
        // Determine if writing or reading based on flags
        if (flags & O_WRONLY || flags & O_CREAT)
        {
            return STDOUT_FILENO;
        }
        else
        {
            return STDIN_FILENO;
        }
    }
}

// Implementation of create_archive
void create_archive(const char *archive_filename, char **members, int count, int verbose)
{
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    mode_t mode = 0644;
    int archive_fd = open_archive(archive_filename, flags, mode);

    // Set permissions explicitly if archive_fd is not stdout
    if (archive_filename && fchmod(archive_fd, mode) == -1)
    {
        perror("Error setting archive file permissions");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }

    handle_verbose(verbose, "Archive file opened/created successfully.");

    // Write VIKTAR_TAG
    if (write(archive_fd, VIKTAR_TAG, strlen(VIKTAR_TAG)) != (ssize_t)strlen(VIKTAR_TAG))
    {
        perror("Error writing VIKTAR_TAG");
        if (archive_filename)
            close(archive_fd);
        exit(EXIT_FAILURE);
    }
    handle_verbose(verbose, "VIKTAR_TAG written to archive.");

    // Iterate over member files
    
    for (int i = 0; i < count; i++)
    {
        const char *filename = members[i];
        struct stat st;
        viktar_header_t header;
        MD5_CTX md5_ctx_header;
        unsigned char md5_header[MD5_DIGEST_LENGTH];
        int member_fd;
        char *data;
        ssize_t bytes_read;
        MD5_CTX md5_ctx_data;
        unsigned char md5_data[MD5_DIGEST_LENGTH];
        viktar_footer_t footer;

        if (stat(filename, &st) == -1)
        {
            fprintf(stderr, "Error: Cannot stat file '%s': %s\n", filename, strerror(errno));
            continue; // Skip this file
        }

        // Populate viktar_header_t
        memset(&header, 0, sizeof(viktar_header_t));
        strncpy(header.viktar_name, filename, VIKTAR_MAX_FILE_NAME_LEN - 1); // Ensure null-termination
        header.st_size = st.st_size;
        header.st_mode = st.st_mode;
        header.st_uid = st.st_uid;
        header.st_gid = st.st_gid;
        header.st_atim = st.st_atim;
        header.st_mtim = st.st_mtim;

        // Calculate MD5 for header
        MD5Init(&md5_ctx_header);
        MD5Update(&md5_ctx_header, (unsigned char *)&header, sizeof(viktar_header_t));
        MD5Final(md5_header, &md5_ctx_header);

        // Open member file
        member_fd = open(filename, O_RDONLY);
        if (member_fd == -1)
        {
            fprintf(stderr, "Error: Cannot open file '%s' for reading: %s\n", filename, strerror(errno));
            continue; // Skip this file
        }

        // Read file data
        data = malloc(st.st_size);
        if (!data)
        {
            fprintf(stderr, "Error: Memory allocation failed for file '%s'\n", filename);
            close(member_fd);
            if (archive_filename)
                close(archive_fd);
            exit(EXIT_FAILURE);
        }

        bytes_read = read(member_fd, data, st.st_size);
        if (bytes_read != st.st_size)
        {
            fprintf(stderr, "Error: Failed to read data from file '%s'\n", filename);
            free(data);
            close(member_fd);
            continue; // Skip this file
        }
        close(member_fd);

        // Calculate MD5 for data
        MD5Init(&md5_ctx_data);
        MD5Update(&md5_ctx_data, (unsigned char *)data, st.st_size);
        MD5Final(md5_data, &md5_ctx_data);

        // Write header to archive
        if (write(archive_fd, &header, sizeof(viktar_header_t)) != sizeof(viktar_header_t))
        {
            fprintf(stderr, "Error: Failed to write header for file '%s'\n", filename);
            free(data);
            continue; // Skip this file
        }
        handle_verbose(verbose, "Header written to archive.");

        // Write data to archive
        if (write(archive_fd, data, st.st_size) != st.st_size)
        {
            fprintf(stderr, "Error: Failed to write data for file '%s'\n", filename);
            free(data);
            continue; // Skip this file
        }
        handle_verbose(verbose, "Data written to archive.");

        // Prepare and write footer
        memcpy(footer.md5sum_header, md5_header, MD5_DIGEST_LENGTH);
        memcpy(footer.md5sum_data, md5_data, MD5_DIGEST_LENGTH);

        if (write(archive_fd, &footer, sizeof(viktar_footer_t)) != sizeof(viktar_footer_t))
        {
            fprintf(stderr, "Error: Failed to write footer for file '%s'\n", filename);
            free(data);
            continue; // Skip this file
        }
        handle_verbose(verbose, "Footer written to archive.");

        free(data);
        handle_verbose(verbose, "File archived successfully.");
    }

    // Close the archive file if it's not stdout
    if (archive_filename)
    {
        if (close(archive_fd) == -1)
        {
            perror("Error closing archive file");
            exit(EXIT_FAILURE);
        }
        handle_verbose(verbose, "Archive file closed successfully.");
    }
}

// Implementation of extract_archive
void extract_archive(const char *archive_filename, char **members, int count, int verbose)
{
    int flags = O_RDONLY;
    int archive_fd = open_archive(archive_filename, flags, 0);
    char tag_buf[sizeof(VIKTAR_TAG)];
    ssize_t tag_read;
    int extract_all;

    handle_verbose(verbose, "Archive file opened successfully for extraction.");

    // Read and validate VIKTAR_TAG
    memset(tag_buf, 0, sizeof(tag_buf));
    tag_read = read(archive_fd, tag_buf, strlen(VIKTAR_TAG));
    if (tag_read != (ssize_t)strlen(VIKTAR_TAG))
    {
        fprintf(stderr, "Error: Failed to read VIKTAR_TAG.\n");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }
    if (strncmp(tag_buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0)
    {
        fprintf(stderr, "Error: Invalid archive format.\n");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }
    handle_verbose(verbose, "VIKTAR_TAG validated successfully.");

    // Determine if all files should be extracted
    extract_all = (count == 0);
    if (extract_all)
    {
        handle_verbose(verbose, "Extracting all files.");
    }

    while (1)
    {
        viktar_header_t header;
        ssize_t bytes_read;
        char *data;
        ssize_t data_read;
        viktar_footer_t footer;
        MD5_CTX md5_ctx_header;
        unsigned char computed_md5_header[MD5_DIGEST_LENGTH];
        MD5_CTX md5_ctx_data;
        unsigned char computed_md5_data[MD5_DIGEST_LENGTH];
        int header_valid;
        int data_valid;
        int should_extract;
        int i;

        // Read header
        bytes_read = read(archive_fd, &header, sizeof(viktar_header_t));
        if (bytes_read == 0)
        {
            break; // End of archive
        }
        if (bytes_read != sizeof(viktar_header_t))
        {
            fprintf(stderr, "Error: Failed to read file header.\n");
            break;
        }

        // Read data
        data = malloc(header.st_size);
        if (!data)
        {
            fprintf(stderr, "Error: Memory allocation failed for file '%s'\n", header.viktar_name);
            close(archive_fd);
            exit(EXIT_FAILURE);
        }
        data_read = read(archive_fd, data, header.st_size);
        if (data_read != header.st_size)
        {
            fprintf(stderr, "Error: Failed to read data for file '%s'\n", header.viktar_name);
            free(data);
            break;
        }

        // Read footer
        if (read(archive_fd, &footer, sizeof(viktar_footer_t)) != sizeof(viktar_footer_t))
        {
            fprintf(stderr, "Error: Failed to read footer for file '%s'\n", header.viktar_name);
            free(data);
            break;
        }

        // Calculate MD5 checksums
        MD5Init(&md5_ctx_header);
        MD5Update(&md5_ctx_header, (unsigned char *)&header, sizeof(viktar_header_t));
        MD5Final(computed_md5_header, &md5_ctx_header);

        MD5Init(&md5_ctx_data);
        MD5Update(&md5_ctx_data, (unsigned char *)data, header.st_size);
        MD5Final(computed_md5_data, &md5_ctx_data);

        // Compare MD5 checksums
        header_valid = (memcmp(footer.md5sum_header, computed_md5_header, MD5_DIGEST_LENGTH) == 0);
        data_valid = (memcmp(footer.md5sum_data, computed_md5_data, MD5_DIGEST_LENGTH) == 0);

        if (!header_valid || !data_valid)
        {
            fprintf(stderr, "Warning: MD5 mismatch for file '%s'.\n", header.viktar_name);
        }

        // Determine if this file should be extracted
        should_extract = extract_all;
        if (!extract_all)
        {
            for (i = 0; i < count; i++)
            {
                if (strncmp(header.viktar_name, members[i], VIKTAR_MAX_FILE_NAME_LEN) == 0)
                {
                    should_extract = 1;
                    break;
                }
            }
        }

        if (should_extract)
        {
            // Truncate filename if necessary
            char filename[VIKTAR_MAX_FILE_NAME_LEN + 1];
            int out_fd;
            ssize_t write_bytes;
            struct timespec times[2];

            strncpy(filename, header.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
            filename[VIKTAR_MAX_FILE_NAME_LEN] = '\0'; // Ensure null-termination

            handle_verbose(verbose, "Extracting file.");

            if (verbose)
            {
                fprintf(stderr, "Extracting: %s\n", filename);
            }

            // Open/Create the extracted file
            out_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, header.st_mode & 0777);
            if (out_fd == -1)
            {
                fprintf(stderr, "Error: Cannot create file '%s': %s\n", filename, strerror(errno));
                free(data);
                continue; // Skip to next file
            }

            // Write data to the file
            write_bytes = write(out_fd, data, header.st_size);
            if (write_bytes != header.st_size)
            {
                fprintf(stderr, "Error: Failed to write data to file '%s'\n", filename);
                close(out_fd);
                free(data);
                continue; // Skip to next file
            }

            // Restore permissions
            if (fchmod(out_fd, header.st_mode) == -1)
            {
                fprintf(stderr, "Error: Failed to set permissions for file '%s'\n", filename);
            }

            // Restore timestamps
            times[0] = header.st_atim;
            times[1] = header.st_mtim;
            if (futimens(out_fd, times) == -1)
            {
                fprintf(stderr, "Error: Failed to set timestamps for file '%s'\n", filename);
            }

            close(out_fd);
            handle_verbose(verbose, "File extracted successfully.");

            if (!header_valid || !data_valid)
            {
                fprintf(stderr, "*** Validation failure: %s for member %s\n",
                        archive_filename ? archive_filename : "stdin", filename);
            }
        }

        free(data);
    }

    // Close the archive file if it's not stdin
    if (archive_filename)
    {
        if (close(archive_fd) == -1)
        {
            perror("Error closing archive file");
            exit(EXIT_FAILURE);
        }
        handle_verbose(verbose, "Archive file closed successfully.");
    }
}

// Implementation of short_toc
void short_toc(const char *archive_filename, int verbose)
{
    int flags = O_RDONLY;
    int archive_fd = open_archive(archive_filename, flags, 0);
    char tag_buf[sizeof(VIKTAR_TAG)];
    ssize_t tag_read;
    memset(tag_buf, 0, sizeof(tag_buf));

    handle_verbose(verbose, "Archive file opened successfully for short TOC.");

    // Read and validate VIKTAR_TAG
    tag_read = read(archive_fd, tag_buf, strlen(VIKTAR_TAG));
    if (tag_read != (ssize_t)strlen(VIKTAR_TAG))
    {
        fprintf(stderr, "Error: Failed to read VIKTAR_TAG.\n");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }
    if (strncmp(tag_buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0)
    {
        fprintf(stderr, "Error: Invalid archive format.\n");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }
    handle_verbose(verbose, "VIKTAR_TAG validated successfully.");

    printf("Contents of viktar file: \"%s\"\n", archive_filename ? archive_filename : "stdin");

    while (1)
    {
        viktar_header_t header;
        ssize_t bytes_read;
        off_t skip_bytes;
        char filename[VIKTAR_MAX_FILE_NAME_LEN + 1];

        // Read header
        bytes_read = read(archive_fd, &header, sizeof(viktar_header_t));
        if (bytes_read == 0)
        {
            break; // End of archive
        }
        if (bytes_read != sizeof(viktar_header_t))
        {
            fprintf(stderr, "Error: Failed to read file header.\n");
            break;
        }

        // Read data and footer to skip
        skip_bytes = header.st_size + sizeof(viktar_footer_t);
        if (lseek(archive_fd, skip_bytes, SEEK_CUR) == -1)
        {
            perror("Error seeking in archive");
            break;
        }

        // Truncate filename if necessary
        strncpy(filename, header.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
        filename[VIKTAR_MAX_FILE_NAME_LEN] = '\0'; // Ensure null-termination

        printf("\tfile name: %s\n", filename);
    }

    // Close the archive file if it's not stdin
    if (archive_filename)
    {
        if (close(archive_fd) == -1)
        {
            perror("Error closing archive file");
            exit(EXIT_FAILURE);
        }
        handle_verbose(verbose, "Archive file closed successfully.");
    }
}

// Implementation of long_toc
void long_toc(const char *archive_filename, int verbose)
{
    int flags = O_RDONLY;
    int archive_fd = open_archive(archive_filename, flags, 0);
    char tag_buf[sizeof(VIKTAR_TAG)];
    ssize_t tag_read;
    memset(tag_buf, 0, sizeof(tag_buf));
    verbose = verbose;

    // Print the archive file name
    printf("Contents of viktar file: %s\n", archive_filename ? archive_filename : "stdin");

    // Read and validate VIKTAR_TAG
    tag_read = read(archive_fd, tag_buf, strlen(VIKTAR_TAG));
    if (tag_read != (ssize_t)strlen(VIKTAR_TAG) || strncmp(tag_buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0)
    {
        fprintf(stderr, "not a viktar file: \"%s\"\n", archive_filename ? archive_filename : "stdin");
        close(archive_fd);
        exit(1);
    }

    while (1)
    {
        viktar_header_t header;
        ssize_t bytes_read;
        char filename[VIKTAR_MAX_FILE_NAME_LEN + 1];
        char perms[11];
        char mtime_str[100];
        char atime_str[100];
        struct tm tm_info;
        mode_t mode;
        struct passwd *pwd;
        struct group *grp;
        viktar_footer_t footer;
        off_t skip_bytes;
        int i;

        // Read header
        bytes_read = read(archive_fd, &header, sizeof(viktar_header_t));
        if (bytes_read == 0)
        {
            break; // End of archive
        }
        if (bytes_read != sizeof(viktar_header_t))
        {
            fprintf(stderr, "Error: Failed to read file header.\n");
            break;
        }

        // Truncate filename if necessary
        strncpy(filename, header.viktar_name, VIKTAR_MAX_FILE_NAME_LEN);
        filename[VIKTAR_MAX_FILE_NAME_LEN] = '\0'; // Ensure null-termination

        // Skip over data
        skip_bytes = header.st_size;
        if (lseek(archive_fd, skip_bytes, SEEK_CUR) == -1)
        {
            perror("Error seeking in archive");
            break;
        }

        // Read footer
        bytes_read = read(archive_fd, &footer, sizeof(viktar_footer_t));
        if (bytes_read != sizeof(viktar_footer_t))
        {
            fprintf(stderr, "Error: Failed to read file footer.\n");
            break;
        }

        // Format permissions
        memset(perms, '-', 10);
        perms[10] = '\0';
        mode = header.st_mode;
        perms[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l'
                                     : S_ISCHR(mode)   ? 'c'
                                     : S_ISBLK(mode)   ? 'b'
                                     : S_ISFIFO(mode)  ? 'p'
                                     : S_ISSOCK(mode)  ? 's'
                                                       : '-';
        perms[1] = (mode & S_IRUSR) ? 'r' : '-';
        perms[2] = (mode & S_IWUSR) ? 'w' : '-';
        perms[3] = (mode & S_IXUSR) ? 'x' : '-';
        perms[4] = (mode & S_IRGRP) ? 'r' : '-';
        perms[5] = (mode & S_IWGRP) ? 'w' : '-';
        perms[6] = (mode & S_IXGRP) ? 'x' : '-';
        perms[7] = (mode & S_IROTH) ? 'r' : '-';
        perms[8] = (mode & S_IWOTH) ? 'w' : '-';
        perms[9] = (mode & S_IXOTH) ? 'x' : '-';

        // Format timestamps
        localtime_r(&header.st_mtim.tv_sec, &tm_info);
        strftime(mtime_str, sizeof(mtime_str), "%Y-%m-%d %H:%M:%S %Z", &tm_info);

        localtime_r(&header.st_atim.tv_sec, &tm_info);
        strftime(atime_str, sizeof(atime_str), "%Y-%m-%d %H:%M:%S %Z", &tm_info);

        // Get username and group name
        pwd = getpwuid(header.st_uid);
        grp = getgrgid(header.st_gid);

        // Print file info
        printf("\tfile name: %s\n", filename);
        printf("\t\tmode:\t\t%s\n", perms);
        printf("\t\tuser:\t\t%s\n", pwd ? pwd->pw_name : "unknown");
        printf("\t\tgroup:\t\t%s\n", grp ? grp->gr_name : "unknown");
        printf("\t\tsize:\t\t%ld\n", (long)header.st_size);
        printf("\t\tmtime:\t\t%s\n", mtime_str);
        printf("\t\tatime:\t\t%s\n", atime_str);

        // Print MD5 sums
        printf("\t\tmd5 sum header:\t");
        for (i = 0; i < MD5_DIGEST_LENGTH; i++)
            printf("%02x", footer.md5sum_header[i]);
        printf("\n");
        printf("\t\tmd5 sum data:\t");
        for (i = 0; i < MD5_DIGEST_LENGTH; i++)
            printf("%02x", footer.md5sum_data[i]);
        printf("\n");
    }

    // Close the archive file if it's not stdin
    if (archive_filename)
    {
        if (close(archive_fd) == -1)
        {
            perror("Error closing archive file");
            exit(EXIT_FAILURE);
        }
    }
}

// Implementation of validate_archive
void validate_archive(const char *archive_filename, int verbose)
{
    int flags = O_RDONLY;
    int archive_fd = open_archive(archive_filename, flags, 0);
    char tag_buf[sizeof(VIKTAR_TAG)];
    ssize_t tag_read;
    int member_count = 1;

    handle_verbose(verbose, "Archive file opened successfully for validation.");

    // Read and validate VIKTAR_TAG
    memset(tag_buf, 0, sizeof(tag_buf));
    tag_read = read(archive_fd, tag_buf, strlen(VIKTAR_TAG));
    if (tag_read != (ssize_t)strlen(VIKTAR_TAG))
    {
        fprintf(stderr, "Error: Failed to read VIKTAR_TAG.\n");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }
    if (strncmp(tag_buf, VIKTAR_TAG, strlen(VIKTAR_TAG)) != 0)
    {
        fprintf(stderr, "Error: Invalid archive format.\n");
        close(archive_fd);
        exit(EXIT_FAILURE);
    }
    handle_verbose(verbose, "VIKTAR_TAG validated successfully.");

    while (1)
    {
        viktar_header_t header;
        ssize_t bytes_read;
        char *data;
        ssize_t data_read;
        viktar_footer_t footer;
        MD5_CTX md5_ctx_header;
        unsigned char computed_md5_header[MD5_DIGEST_LENGTH];
        MD5_CTX md5_ctx_data;
        unsigned char computed_md5_data[MD5_DIGEST_LENGTH];
        int header_valid;
        int data_valid;
        int i;

        // Read header
        bytes_read = read(archive_fd, &header, sizeof(viktar_header_t));
        if (bytes_read == 0)
        {
            break; // End of archive
        }
        if (bytes_read != sizeof(viktar_header_t))
        {
            fprintf(stderr, "Error: Failed to read file header.\n");
            break;
        }

        // Read data
        data = malloc(header.st_size);
        if (!data)
        {
            fprintf(stderr, "Error: Memory allocation failed for file '%s'\n", header.viktar_name);
            close(archive_fd);
            exit(EXIT_FAILURE);
        }
        data_read = read(archive_fd, data, header.st_size);
        if (data_read != header.st_size)
        {
            fprintf(stderr, "Error: Failed to read data for file '%s'\n", header.viktar_name);
            free(data);
            break;
        }

        // Read footer
        if (read(archive_fd, &footer, sizeof(viktar_footer_t)) != sizeof(viktar_footer_t))
        {
            fprintf(stderr, "Error: Failed to read footer for file '%s'\n", header.viktar_name);
            free(data);
            break;
        }

        // Calculate MD5 checksums
        MD5Init(&md5_ctx_header);
        MD5Update(&md5_ctx_header, (unsigned char *)&header, sizeof(viktar_header_t));
        MD5Final(computed_md5_header, &md5_ctx_header);

        MD5Init(&md5_ctx_data);
        MD5Update(&md5_ctx_data, (unsigned char *)data, header.st_size);
        MD5Final(computed_md5_data, &md5_ctx_data);

        // Compare MD5 checksums
        header_valid = (memcmp(footer.md5sum_header, computed_md5_header, MD5_DIGEST_LENGTH) == 0);
        data_valid = (memcmp(footer.md5sum_data, computed_md5_data, MD5_DIGEST_LENGTH) == 0);

        // Print validation results
        printf("Validation for data member %d:\n", member_count);
        if (!header_valid)
        {
            printf("*** Header MD5 does not match:\n");
            printf("Computed MD5: ");
            for (i = 0; i < MD5_DIGEST_LENGTH; i++)
                printf("%02x", computed_md5_header[i]);
            printf("\n");
            printf("Stored MD5:  ");
            for (i = 0; i < MD5_DIGEST_LENGTH; i++)
                printf("%02x", footer.md5sum_header[i]);
            printf("\n");
        }
        if (!data_valid)
        {
            printf("*** Data MD5 does not match:\n");
            printf("Computed MD5: ");
            for (i = 0; i < MD5_DIGEST_LENGTH; i++)
                printf("%02x", computed_md5_data[i]);
            printf("\n");
            printf("Stored MD5:  ");
            for (i = 0; i < MD5_DIGEST_LENGTH; i++)
                printf("%02x", footer.md5sum_data[i]);
            printf("\n");
        }
        if (header_valid && data_valid)
        {
            printf("Member %d is valid.\n", member_count);
        }
        else
        {
            printf("*** Validation failure: %s for member %d\n",
                   archive_filename ? archive_filename : "stdin", member_count);
        }
        member_count++;

        free(data);
    }

    // Close the archive file if it's not stdin
    if (archive_filename)
    {
        if (close(archive_fd) == -1)
        {
            perror("Error closing archive file");
            exit(EXIT_FAILURE);
        }
        handle_verbose(verbose, "Archive file closed successfully.");
    }
}

// Implementation of main
int main(int argc, char *argv[])
{
    int opt;
    char *archive_filename = NULL;
    int verbose = 0;
    viktar_action_t action = ACTION_NONE; // Initialize to no action
    int remaining_args;
    char **members;

    // Parse command-line options
    while ((opt = getopt(argc, argv, OPTIONS)) != -1)
    {
        switch (opt)
        {
        case 'c':
            action = ACTION_CREATE;
            break;
        case 'x':
            action = ACTION_EXTRACT;
            break;
        case 't':
            action = ACTION_TOC_SHORT;
            break;
        case 'T':
            action = ACTION_TOC_LONG;
            break;
        case 'V':
            action = ACTION_VALIDATE;
            break;
        case 'f':
            archive_filename = optarg;
            break;
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case 'v':
            verbose = 1;
            break;
        case '?':
            fprintf(stderr, "Warning: Unknown option '-%c' ignored.\n", optopt);
            break;
        default:
            print_help();
            exit(EXIT_FAILURE);
        }
    }

    // Remaining command-line arguments after options
    remaining_args = argc - optind;
    members = &argv[optind];

    // Handle actions based on the last specified action
    switch (action)
    {
    case ACTION_CREATE:
        create_archive(archive_filename, members, remaining_args, verbose);
        break;
    case ACTION_EXTRACT:
        extract_archive(archive_filename, members, remaining_args, verbose);
        break;
    case ACTION_TOC_SHORT:
        short_toc(archive_filename, verbose);
        break;
    case ACTION_TOC_LONG:
        long_toc(archive_filename, verbose);
        break;
    case ACTION_VALIDATE:
        validate_archive(archive_filename, verbose);
        break;
    default:
        fprintf(stderr, "Error: No valid action specified. Use -h for help.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}