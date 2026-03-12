#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "generator.h"
#include "tar.h"


int run_test(tar_t* a, const char* path) {
    //runs the test and compute the checksum for the tar header 
    calculate_checksum(a);
    write_tar(a);
    execute_extractor(path);
    
    return 0;
}

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
int fuzz_8bits(tar_t* a, const char* path, char* field) {
    //testing negative number : 
    strcpy(field, "-1234567");
    run_test(a, path);

    //testing not number 
    strcpy(field, "abcdefg");
    run_test(a, path);

    //embeded null : 
    memcpy(field, "A\0AAAAA", 8);
    run_test(a, path);

    //not null terminated :
    memset(field, 'A', 8);
    run_test(a, path);
    
    memcpy(field, "   644  ", 8);
    run_test(a, path);

    //All spaces
    memset(field, ' ', 8);
    run_test(a, path);

    memcpy(field, "8888888", 8);
    run_test(a, path);

    // for (int pos = 0; pos < 8; pos++) {
    //     for (int c = 0; c < 256; c++) {
    //         memcpy(field, "0000755", 8); // baseline
    //         field[pos] = c;

    //         run_test(a, path);
    //     }
    // }
    return 0;
}
int generate_inputs(const char* path) {
    (void)path;
    
    tar_t candidate = {0};

    init_valid_tar(&candidate);

    fuzz_8bits(&candidate, path, candidate.mode);
    fuzz_8bits(&candidate, path, candidate.uid);
    fuzz_8bits(&candidate, path, candidate.gid);



    return 0;
}

