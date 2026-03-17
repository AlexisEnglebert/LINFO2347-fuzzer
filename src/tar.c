#include "tar.h"
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>


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

    memset(tar, 0, sizeof(*tar));
    
    strcpy(tar->magic, "ustar");
    strcpy(tar->name, "test");

    strcpy(tar->version, "00");

    return 0;
}

int write_tar(tar_t* data, const char* filename, const char *content, size_t data_size) {
    //careful that data_size is only used for the content, header must previously be correctly initialized
    //this is done bc we need to fuzz size soooooo you get it.
    FILE* fd = fopen(filename, "w");

    if (fd == NULL) {
        fprintf(stderr, "Error while creating %s: %s\n", filename, strerror(errno));
        return - 1;
    }

    int res = fwrite(data, sizeof(tar_t), 1, fd);
    int rv = 0; 
    if (res == 0) {
        rv = -1;
    }
    
    fwrite(content, sizeof(char), data_size, fd);

     /* pad file data to 512-byte boundary */
    /*if (data_size > 0) {
        size_t pad = (512 - (data_size % 512)) % 512;
        if (pad) {
            char pad_buf[512] = {0};
            fwrite(pad_buf, 1, pad, fd);
        }
    }*/

    char end_block[1024];
    memset(&end_block, 0, 1024);
    fwrite(&end_block, sizeof(char), 1024, fd);
    
    res = fclose(fd);
    if (res == -1) {
        rv = -1;
    }
    return rv;
}

int success_cnt = 0;

int save_success_tar(tar_t* tar, const char* content, size_t data_size) {
    char filename[100];
    sprintf(filename, "%s%d%s", "success_", success_cnt, "_archive.tar");
    success_cnt++;
    write_tar(tar, filename, content, data_size);
    return 0;
}