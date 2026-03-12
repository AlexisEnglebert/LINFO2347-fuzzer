#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "generator.h"
#include "tar.h"

int execute_extractor(const char* command_prefix) {
    char executable[100];
    
    strcpy(executable, command_prefix);
    strcat(executable, command);

    strcat(executable, " archive.tar");
    FILE *fp = fp = popen(executable, "r");
    
    if (fp == NULL) {
        printf("Error opening pipe: %s\n", strerror(errno));
        return -1;
    }
    
    char program_output[33];
    int return_value = 0;
    if(fgets(program_output, 33, fp) == NULL) {
        goto close_pipe;
    }

    if(strncmp(program_output, "*** The program has crashed ***\n", 33)) {
        printf("---------- FOUND A BUG --------------\n");
        goto close_pipe;
    } else {
        return_value = -1;
        goto close_pipe;
    }

close_pipe:
    int ret = pclose(fp);
    if(ret == -1) {
        fprintf(stderr, "Error closing pipe: %s\n", strerror(errno));
        return_value = -1;
    }

    return return_value;
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
            if(ascii_char == '/') continue; // On ignore le / de ses mort
            tar->name[pos] = ascii_char;
            calculate_checksum(tar);
            write_tar(tar);
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

int generate_inputs() {
    
    tar_t candidate = {0};
    init_valid_tar(&candidate);
    fuzz_name(&candidate);
    


    return 0;
}

