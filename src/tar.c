#include "tar.h"
#include <errno.h>

/**
 * Computes the checksum for a tar header and encode it on the header
 * @param entry: The tar header
 * @return the value of the checksum
 */

unsigned int calculate_checksum(tar_t* entry) {
    // use spaces for the checksum bytes while calculating the checksum
    memset(entry->chksum, ' ', 8);

    // sum of entire metadata
    unsigned int check = 0;
    unsigned char* raw = (unsigned char*) entry;
    for(int i = 0; i < 512; i++){
        check += raw[i];
    }

    snprintf(entry->chksum, sizeof(entry->chksum), "%06o0", check);

    entry->chksum[6] = '\0';
    entry->chksum[7] = ' ';
    return check;
}


int init_valid_tar(tar_t* tar) {

    strcpy(tar->magic, "ustar");
    strcpy(tar->name, "test");
    strcpy(tar->version, "00");
    calculate_checksum(tar);

    return 0;
}

int write_tar(tar_t* data) {
    char total[100] = {0};
    FILE* fd = fopen("archive.tar", "w");

    if (fd == NULL) {
        fprintf(stderr, "Error while creating %s: %s\n", total, strerror(errno));
        return - 1;
    }

    int res = fwrite(data, sizeof(tar_t), 1, fd);
    int rv = 0; 
    if (res == 0) {
        rv = -1;
    }

    res = fclose(fd);
    if (res == -1) {
        rv = -1;
    }
    return rv;
}