#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "generator.h"
#include "tar.h"

int execute_extractor(const char* path) {
    char command[51];
    strcpy(command, path);
    strncat(command, " archive.tar", 25);

    FILE *fp = fp = popen(command, "r");
    
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
        goto close_pipe;
    } else {
        return_value = -1;
        goto close_pipe;
    }

close_pipe:
    if(pclose(fp) == -1) {
        fprintf(stderr, "Error closing pipe: %s\n", strerror(errno));
        return_value = -1;
    }

    return return_value;
    
}

int permute_name(tar_t* a) {
    write_tar(a);
    execute_extractor("");
    
    return 0;
}

int generate_inputs(const char* path) {
    (void)path;
    
    tar_t candidate = {0};

    init_valid_tar(&candidate);
    // TODO ....
    write_tar(&candidate);

    return 0;
}

