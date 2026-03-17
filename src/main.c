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
    command = strdup(argv[1]);
    
    int ret = generate_inputs();
    
    return ret;
}