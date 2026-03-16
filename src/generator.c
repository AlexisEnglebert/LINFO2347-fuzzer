#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>


#include "generator.h"
#include "tar.h"


int run_test(tar_t* tar, const char* command_prefix, const char* content, size_t data_size) {
    //runs the test and compute the checksum for the tar header 
    calculate_checksum(tar);
    write_tar(tar, "archive.tar", content, data_size);

    if (execute_extractor(command_prefix) == 0) {
        save_sucess_tar(tar, content, data_size);
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
        fprintf(stderr, "Error while changing to %s directory: %s\n", "trash", strerror(errno));
        return -1;
    }

    // Fill name with only A
    for(int i = 0; i < 99; i++) {
        tar->name[i] = 'a';
    }
    tar->name[99] = '\0';
 
    for(int pos = 0; pos < 99; pos++){
        for(size_t ascii_char = 1; ascii_char <= 255; ascii_char++) {
            if(ascii_char == '/') continue; // On ignore le / de ses mort
            tar->name[pos] = ascii_char;
            calculate_checksum(tar);
            write_tar(tar, "archive.tar", "", 0);
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
int fuzz_mode(tar_t* a) {
    //testing negative number : 
    strcpy(a->mode, "-1234567");
    run_test(a, "", "", 0);

    //testing not number 
    strcpy(a->mode, "abcdefg");
    run_test(a, "", "", 0);

    //embeded null : 
    memcpy(a->mode, "A\0AAAAA", 8);
    run_test(a, "", "", 0);

    //not null terminated :
    memset(a->mode, 'A', 8);
    run_test(a, "", "", 0);
    memcpy(a->mode, "   644  ", 8);
    run_test(a, "", "", 0);

    //All spaces
    memset(a->mode, ' ', 8);
    run_test(a, "", "", 0);

    memcpy(a->mode, "8888888", 8);
    run_test(a, "", "", 0);

    for (int pos = 0; pos < 8; pos++) {
        for (int c = 0; c < 256; c++) {
            memcpy(a->mode, "0000755", 8); // baseline
            a->mode[pos] = c;

            run_test(a, "", "", 0);
        }
    }
    return 0;
}

void fuzz_time(tar_t* tar) {
    for(int i = 0; i < 8; i++) {
        for(int val = '1'; val <= '7'; val++) {
            tar->mtime[i] = val;
            run_test(tar, "", "", 0);
        }
    }
}


void fuzz_typeflag(tar_t* tar) {
    strcpy(tar->linkname, "pouetpouet");
    
    tar->typeflag = '1';
    run_test(tar, "", "", 0);
    tar->typeflag = '2';
    run_test(tar, "", "", 0);
    tar->typeflag = '3';
    run_test(tar, "", "", 0);
    tar->typeflag = '4';
    run_test(tar, "", "", 0);
    tar->typeflag = '5';
    run_test(tar, "", "", 0);
    tar->typeflag = '7';
    run_test(tar, "", "", 0);
    tar->typeflag = 'g';
    run_test(tar, "", "", 0);
    tar->typeflag = 'x';
    run_test(tar, "", "", 0);

    for(int i = 65; i < 65+26; i++) {
        tar->typeflag = i;
        run_test(tar, "", "", 0);
    }
}

void fuzz_size_with_empty_file(tar_t* tar) {
    memset(tar->size, 0, 12);
    char content[100];
    memset(content, 'a', 100);
    for(int pos = 0; pos < 11; pos++) {
        for(int val = '1'; val < '8'; val++) {
            tar->size[pos] = val;
            run_test(tar, "", content, 100);
        }
    }
    memset(tar->size, 0, 12);
}


void fuzz_guid(tar_t* tar) {
    //first gid then uid 
    for(int i = 0; i < 7; i++) {
        for(int val = '1'; val <= '7'; val++) {
            tar->gid[i] = val;
            run_test(tar, "", "", 0);
        }
    }
}


int generate_inputs() {
    
    tar_t candidate = {0};
    init_valid_tar(&candidate);
    fuzz_size_with_empty_file(&candidate);
    

    init_valid_tar(&candidate);
    fuzz_typeflag(&candidate);
    
    // init_valid_tar(&candidate);
    // fuzz_time(&candidate);
        
    
    
    // init_valid_tar(&candidate);
    // fuzz_name(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_mode(&candidate);

    init_valid_tar(&candidate);
    fuzz_guid(&candidate);

    return 0;
}

