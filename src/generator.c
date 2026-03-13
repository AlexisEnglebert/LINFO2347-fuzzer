#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>


#include "generator.h"
#include "tar.h"


int run_test(tar_t* tar, const char* command_prefix) {
    //runs the test and compute the checksum for the tar header 
    calculate_checksum(tar);
    write_tar(tar, "archive.tar");
    if (execute_extractor(command_prefix) == 0) {
        save_sucess_tar(tar);
    }
    
    return 0;
}

int execute_extractor(const char* command_prefix) {
    char executable[256];
    
    snprintf(executable, sizeof(executable), "%s%s", command_prefix, command);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Error creating pipe\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Error during fork\n");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            fprintf(stderr, "Error during dup2\n");
            return -1;
        }
        close(pipefd[1]);
        char *args[] = {
            executable,
            "archive.tar",
            NULL
        };
        execvp(executable, args);        
        fprintf(stderr, "Error executing program : %s\n", strerror(errno));
        return -1;
    } else {
        close(pipefd[1]);
        FILE *fp = fdopen(pipefd[0], "r");
        char program_output[64] = {0};
        int rv = -1;

        if (fgets(program_output, sizeof(program_output), fp) != NULL) {
                if (strncmp(program_output, "*** The program has crashed ***", 31) == 0) {
                    printf("---------- FOUND A BUG --------------\n");
                    rv = 0;
                }
            }
        fclose(fp);

        int status;
        waitpid(pid, &status, 0);

        return rv;
    }
    return -1;
}


int fuzz_name(tar_t* tar) {
    mkdir("trash", 0777);

    if (chdir("trash") != 0) {
        fprintf(stderr, "Error while changin to %s directory: %s\n", "trash", strerror(errno));
        return -1;
    }

    // Fill name with only A
    for(int i = 0; i < 99; i++) {
        tar->name[i] = 'a';
    }
    tar->name[99] = '\0';
 
    for(int pos = 0; pos < 99; pos++){
        for(size_t ascii_char = 1; ascii_char <= 255; ascii_char++) {
            //if(ascii_char == '/') continue; // On ignore le / de ses mort
            tar->name[pos] = ascii_char;
            calculate_checksum(tar);
            write_tar(tar, "archive.tar");
            if (execute_extractor("../") == 0) {

            }
            tar->name[pos] = 'a';
        }
    }

    chdir("../");
    system("rm -rf trash");
    printf("finished\n");
    
    return 0;
}
int fuzz_8bits(tar_t* a, char* field) {
    //testing negative number : 
    strcpy(field, "-1234567");
    run_test(a, "");

    //testing not number 
    strcpy(field, "abcdefg");
    run_test(a, "");

    //embeded null : 
    memcpy(field, "A\0AAAAA", 8);
    run_test(a, "");

    //not null terminated :
    memset(field, 'A', 8);
    run_test(a, "");
    memcpy(field, "   644  ", 8);
    run_test(a, "");

    //All spaces
    memset(field, ' ', 8);
    run_test(a, "");

    memcpy(field, "8888888", 8);
    run_test(a, "");

    for (int pos = 0; pos < 8; pos++) {
        for (int c = 0; c < 256; c++) {
            memcpy(field, "0000755", 8); // baseline
            field[pos] = c;

            run_test(a, "");
        }
    }
    return 0;
}

void fuzz_time(tar_t* tar) {
    strcpy(tar->mtime, "-1");
    run_test(tar, "");
}
int generate_inputs() {
    
    tar_t candidate = {0};
    init_valid_tar(&candidate);
    
    fuzz_time(&candidate);

    fuzz_name(&candidate);
    exit(0);

    fuzz_8bits(&candidate, candidate.mode);
    fuzz_8bits(&candidate, candidate.uid);
    fuzz_8bits(&candidate, candidate.gid);


    


    return 0;
}

