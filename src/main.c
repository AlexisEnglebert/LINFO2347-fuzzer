#include <stdio.h>
#include <string.h>

#include "generator.h"

void help() {
    fprintf(stderr, "Usage: ./fuzzer extractor\n");
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        help();
        return -1;
    }

    char cmd[51];
    strncpy(cmd, argv[1], 25);
    cmd[26] = '\0';
    
    int ret = generate_inputs(cmd);
    
    return ret;
}