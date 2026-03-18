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
        save_success_tar(tar, content, data_size);
    }
    
    //remove("test");
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
        fprintf(stderr, "Error executing program %s: %s\n", executable, strerror(errno));
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
    for(int i = 0; i < 1; i++) {
        for(int val = '1'; val <= '9'; val++) {
            tar->gid[i] = val;
            run_test(tar, "", "", 0);
        }
    }

    for(int i = 0; i < 7; i++) {
        for(int val = '1'; val <= '9'; val++) {
            tar->uid[i] = val;
            run_test(tar, "", "", 0);
        }
    }
}

void fuzz_links(tar_t* tar) {
    tar->typeflag = '2'; // Symbolink link
    strcpy(tar->name, "link_trap");
    strcpy(tar->linkname, "../../secret.txt"); // Tentative de sortie
    run_test(tar, "", "", 0);

    // Boucle infinie
    strcpy(tar->name, "loop");
    strcpy(tar->linkname, "loop");
    run_test(tar, "", "", 0);

    //remove files : 
    system("rm -f link_trap");
    system("rm -f loop");
}
void fuzz_checksum(tar_t *tar) {
    // Start from valid tar with proper checksum
    init_valid_tar(tar);
    calculate_checksum(tar);

    char original[8];
    memcpy(original, tar->chksum, 8);

    memset(tar->chksum, ' ', 8);
    run_test(tar, "", "", 0);

    // 2) All '0'
    memset(tar->chksum, '0', 7);
    tar->chksum[7] = ' ';
    run_test(tar, "", "", 0);

    // 3) Very large octal number
    memcpy(tar->chksum, "777777 ", 7);
    tar->chksum[7] = ' ';
    run_test(tar, "", "", 0);
    
    // 4) Non-octal characters
    memcpy(tar->chksum, "ABCDEF ", 7);
    tar->chksum[7] = ' ';
    run_test(tar, "", "", 0);

    // 5) Restore and flip one byte
    memcpy(tar->chksum, original, 8);
    for (int i = 0; i < 7; i++) {
        char saved = tar->chksum[i];
        tar->chksum[i] = (char)(saved ^ 0xFF);
        run_test(tar, "", "", 0);
        tar->chksum[i] = saved;
    }
}
void fuzz_magic(tar_t* tar) {
    const char* magics[] = {"ustar ", "ustar\0", "      ", "\xff\xff\xff\xff\xff"};
    for(int i=0; i<4; i++) {
        memcpy(tar->magic, magics[i], 6);
        run_test(tar, "", "", 0);
    }
}




void fuzz_chains(const char* command_prefix) {
    tar_t h1, h2;
    init_valid_tar(&h1);
    init_valid_tar(&h2);

    FILE* fd = fopen("archive.tar", "wb");
    strcpy(h1.name, "file1.txt");
    strcpy(h1.size, "00000000010");
    calculate_checksum(&h1);
    fwrite(&h1, sizeof(tar_t), 1, fd);
    fwrite("12345678", 1, 512, fd);

    strcpy(h2.name, "file2.txt");
    calculate_checksum(&h2);
    fwrite(&h2, sizeof(tar_t), 1, fd);
    
    char end[1024] = {0};
    fwrite(end, 1, 1024, fd);
    fclose(fd);
    execute_extractor(command_prefix);

    fd = fopen("archive.tar", "wb");
    strcpy(h1.size, "00000000100");
    calculate_checksum(&h1);
    fwrite(&h1, sizeof(tar_t), 1, fd); 
    fwrite(&h2, sizeof(tar_t), 1, fd);
    fclose(fd);
    execute_extractor(command_prefix);

    //remove files : 
    system("rm -f archive.tar");
    system("rm -f file1.txt");
    system("rm -f file2.txt");
}


void fuzz_extreme_sizes(tar_t* tar) {
    strcpy(tar->size, "77777777777"); 
    run_test(tar, "", "", 0);

    strcpy(tar->size, "-0000000001");
    run_test(tar, "", "", 0);

    strcpy(tar->size, "00000000A10");
    run_test(tar, "", "", 0);
}

void fuzz_null_archive() {
    // Test 1: Fichier vide
    FILE* f = fopen("archive.tar", "wb");
    fclose(f);
    execute_extractor("");
}

void fuzz_uname_gname(tar_t* tar) {
    init_valid_tar(tar);

    memset(tar->uname, 'B', 31); // Pas de \0 final
    memset(tar->gname, 'C', 31); // Pas de \0 final
    run_test(tar, "", "", 0);
}

void fuzz_circular_links(tar_t* tar) {
    init_valid_tar(tar);
    tar->typeflag = '2'; // Symlink
    strcpy(tar->name, "self_trap");
    strcpy(tar->linkname, "self_trap");
    run_test(tar, "", "", 0);

    FILE* fd = fopen("archive.tar", "wb");
    tar_t h1, h2;
    init_valid_tar(&h1);
    init_valid_tar(&h2);

    h1.typeflag = '2';
    strcpy(h1.name, "A");
    strcpy(h1.linkname, "B");
    calculate_checksum(&h1);
    fwrite(&h1, sizeof(tar_t), 1, fd);

    h2.typeflag = '2';
    strcpy(h2.name, "B");
    strcpy(h2.linkname, "A");
    calculate_checksum(&h2);
    fwrite(&h2, sizeof(tar_t), 1, fd);

    char end[1024] = {0};
    fwrite(end, 1, 1024, fd);
    fclose(fd);

    if (execute_extractor("") == 0) {
        system("cp archive.tar success_circular_link.tar");
    }
    system("rm -f archive.tar");
    system("rm -f A");
    system("rm -f B");
    system("rm -f self_trap");

}

void fuzz_extreme_octal_size(tar_t* tar) {
    init_valid_tar(tar);
    // grande valeur
    strcpy(tar->size, "77777777777"); 
    run_test(tar, "", "", 0);
    
    // non octal aussi
    strcpy(tar->size, "0000000A123");
    run_test(tar, "", "", 0);
}


void fuzz_metadata_overflow(tar_t* tar) {
    init_valid_tar(tar);
    memset(tar->uname, 'X', 32); // Rempli sans \0 
    memset(tar->gname, 'Y', 32); // Rempli sans \0 
    run_test(tar, "", "", 0);

    //TODO test with edge case (invalid uname or gname)
}

void fuzz_weird_types(tar_t* tar) {
    char bad_types[] = {0, 1, 127, 255, 'Z', '\0'};
    for(int i = 0; i < 6; i++) {
        init_valid_tar(tar);
        tar->typeflag = bad_types[i];
        run_test(tar, "", "", 0);
    }
}

void fuzz_base256_size(tar_t* tar) {
    //adding manually the 0x80 prefix to indicate base-256 encoding for size field instead of octal, and then filling the rest with 0xFF to get a huge number
    // 0x80 prefix = positive base-256
    tar->size[0] = 0x80;
    memset(tar->size + 1, 0xFF, 10); // Huge number
    run_test(tar, "", "", 0);

    // 0xFF prefix = negative (two's complement)
    memset(tar->size, 0xFF, 11);
    run_test(tar, "", "", 0);

    // Also try on uid/gid/mtime fields
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! THIS IS BUG3
    tar->uid[0] = 0x80;
    memset(tar->uid + 1, 0xFF, 7);
    run_test(tar, "", "", 0);

    tar->gid[0] = 0x80;
    memset(tar->gid + 1, 0xFF, 7);
    run_test(tar, "", "", 0);

    tar->mtime[0] = 0x80;
    memset(tar->mtime + 1, 0xFF, 11);
    run_test(tar, "", "", 0);

    tar->uname[0] = 0x80;
    tar->gname[0] = 0x80;
    memset(tar->uname+1, 0xFF, 30); // Rempli sans \0 
    memset(tar->gname+1, 0XFF, 30); // Rempli sans \0 
}

void fuzz_path_traversal_prefix_name(tar_t* tar) {
    init_valid_tar(tar);
    strcpy(tar->prefix, "../../../");
    strcpy(tar->name, "etc/passwd");
    run_test(tar, "", "", 0);
}

void fuzz_short_name(tar_t* tar) {
    strcpy(tar->name,"A");
    char content[2000];
    memset(content, 'B', 100);
    run_test(tar, "", content, 2000);
    remove("A");
}

void fuzz_same_file_in_tar(tar_t* tar) {
    tar_t h1 = *tar;
    tar_t h2 = *tar;

    strcpy(h1.name, "test");
    strcpy(h2.name, "test");
    h1.typeflag = '0';
    h2.typeflag = '0';
    memset(h1.linkname, 0, sizeof(h1.linkname));
    memset(h2.linkname, 0, sizeof(h2.linkname));

    const unsigned int size1 = 1;
    const unsigned int size2 = 1;
    snprintf(h1.size, sizeof(h1.size), "%011o", size1);
    snprintf(h2.size, sizeof(h2.size), "%011o", size2);

    calculate_checksum(&h1);
    calculate_checksum(&h2);

   
    char* content = calloc(1, sizeof(*content)+sizeof(tar_t)+512+512);
    content[0] = 'A';
    memcpy(content+512, (void*)&h2, sizeof(h2));
    content[512+sizeof(h2)] = 'A';
    
    run_test(&h1, "", content, sizeof(*content)+sizeof(tar_t)+512+512);

    free(content);
    //remove("test");
}

void fuzz_version(tar_t* tar) {
    for(int i = '0';  i <= '9'; i++) {
        for(int j = '0'; j <= '9'; j++) {
            tar->version[0] = i;
            tar->version[1] = j;
            run_test(tar, "", "" ,0);
        }
    }
}

int generate_inputs() {
    
    tar_t candidate = {0};

    init_valid_tar(&candidate);
    fuzz_version(&candidate);
    
    init_valid_tar(&candidate);
    fuzz_guid(&candidate);
    
    init_valid_tar(&candidate);
    fuzz_typeflag(&candidate);

    init_valid_tar(&candidate);
    fuzz_size_with_empty_file(&candidate);

    init_valid_tar(&candidate);
    fuzz_same_file_in_tar(&candidate);

    init_valid_tar(&candidate);
    fuzz_base256_size(&candidate);

    /*
    init_valid_tar(&candidate);
    fuzz_time(&candidate);
     
    init_valid_tar(&candidate);
    fuzz_magic_version(&candidate);

    */
    
    // init_valid_tar(&candidate);
    // fuzz_name(&candidate);

   
    /*init_valid_tar(&candidate);
    fuzz_mode(&candidate);


    init_valid_tar(&candidate);
    fuzz_checksum(&candidate);

    init_valid_tar(&candidate);
    fuzz_links(&candidate);

    init_valid_tar(&candidate);
    fuzz_chains("");

    init_valid_tar(&candidate);
    fuzz_extreme_sizes(&candidate);

    init_valid_tar(&candidate);
    fuzz_circular_links(&candidate);

    */
    /*init_valid_tar(&candidate);
    fuzz_uname_gname(&candidate);*/
/*

    init_valid_tar(&candidate);
    fuzz_extreme_octal_size(&candidate);

    init_valid_tar(&candidate);
    fuzz_metadata_overflow(&candidate);

    init_valid_tar(&candidate);
    fuzz_weird_types(&candidate);

    fuzz_null_archive();

    //bug 3

    init_valid_tar(&candidate);
    fuzz_path_traversal_prefix_name(&candidate);
*/

    init_valid_tar(&candidate);
    fuzz_short_name(&candidate);
    return 0;
}
