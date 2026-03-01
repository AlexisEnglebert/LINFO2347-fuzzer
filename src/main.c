#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "help.h"

void help() {
    fprintf(stderr, "Usage: ./fuzzer cmd\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        help();
        return -1;
    }

    int rv = 0;
    char cmd[51];
    strncpy(cmd, argv[1], 25);
    cmd[26] = '\0';
    strncat(cmd, " archive.tar", 25);
    char buf[33];

    FILE *fp = fp = popen(cmd, "r");
    
    if (fp == NULL) {
        printf("Error opening pipe: %s\n", strerror(errno));
        return -1;
    }

    if(fgets(buf, 33, fp) == NULL) {
        printf("No output\n");
        goto finally;
    }

    if(strncmp(buf, "*** The program has crashed ***\n", 33)) {
        printf("Not the crash message\n");
        goto finally;
    } else {
        printf("Crash message\n");
        rv = 1;
        goto finally;
    }
finally:
    if(pclose(fp) == -1) {
        fprintf(stderr, "Error closing pipe: %s\n", strerror(errno));
        rv = -1;
    }
    
    return rv;
}