#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "generator.h"


void help() {
    fprintf(stderr, "Usage: ./fuzzer extractor\n");
}

char* command = NULL;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        help();
        return -1;
    }

    command = calloc(51, sizeof(*command));
    
    strncpy(command, argv[1], 25);
    command[26] = '\0';
    
    int ret = generate_inputs();
    
    return ret;
}