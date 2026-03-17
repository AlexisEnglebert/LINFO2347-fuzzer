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
        fprintf(stderr, "Error executing program : %s\n", strerror(errno));
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
    tar->typeflag = '1';
    run_test(tar, "", "", 0);
    exit(0);
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

    strcpy(tar->size, "0000000000");
    for(int pos = 1; pos < 12; pos++) {
        for(int val = '1'; val < '8'; val++) {
            tar->size[11-pos] = val ;
            printf("Testing size : %s\n", tar->size);
            char *content = malloc(512);
            memset(content, 'A', 511);
            memset(content + 511, '\0', 1);
            run_test(tar, "", content, 512);
        }
    }
    strcpy(tar->size, "000000000000");
}

void fuzz_guid(tar_t* tar) {
    //first gid then uid 
    for(int i = 0; i < 7; i++) {
        for(int val = '1'; val <= '7'; val++) {
            tar->gid[i] = val;
            run_test(tar, "", "", 0);
        }
    }
    for(int i = 0; i < 7; i++) {
        for(int val = '1'; val <= '7'; val++) {
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
void fuzz_magic_version(tar_t* tar) {
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

    // Cas 1 : Deux fichiers valides à la suite
    FILE* fd = fopen("archive.tar", "wb");
    strcpy(h1.name, "file1.txt");
    strcpy(h1.size, "00000000010"); // 8 octets en octal
    calculate_checksum(&h1);
    fwrite(&h1, sizeof(tar_t), 1, fd);
    fwrite("12345678", 1, 512, fd); // Data + Padding

    strcpy(h2.name, "file2.txt");
    calculate_checksum(&h2);
    fwrite(&h2, sizeof(tar_t), 1, fd);
    
    char end[1024] = {0};
    fwrite(end, 1, 1024, fd);
    fclose(fd);
    execute_extractor(command_prefix);

    // Cas 2 : Header fantôme (Header -> Header sans données)
    fd = fopen("archive.tar", "wb");
    strcpy(h1.size, "00000000100"); // Prétend avoir des données
    calculate_checksum(&h1);
    fwrite(&h1, sizeof(tar_t), 1, fd); 
    fwrite(&h2, sizeof(tar_t), 1, fd); // On colle le 2ème header direct !
    fclose(fd);
    execute_extractor(command_prefix);

    //remove files : 
    system("rm -f archive.tar");
    system("rm -f file1.txt");
    system("rm -f file2.txt");
}


void fuzz_extreme_sizes(tar_t* tar) {
    // 1. Très grand nombre octal (proche de la limite max)
    strcpy(tar->size, "77777777777"); 
    run_test(tar, "", "", 0);

    // 2. Nombre négatif en octal (certains parseurs détestent ça)
    strcpy(tar->size, "-0000000001");
    run_test(tar, "", "", 0);

    // 3. Size avec des lettres (force l'erreur de parsing)
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
    memset(tar->uname, 'B', 32); // Pas de \0 final
    memset(tar->gname, 'C', 32); // Pas de \0 final
    run_test(tar, "", "", 0);
}




void fuzz_circular_links(tar_t* tar) {
    // Cas 1 : Lien vers soi-même direct
    init_valid_tar(tar);
    tar->typeflag = '2'; // Symlink
    strcpy(tar->name, "self_trap");
    strcpy(tar->linkname, "self_trap");
    run_test(tar, "", "", 0);

    // Cas 2 : cycle (A -> B et B -> A)
    // plusieurs fichiers pour faire le cycle
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
}

void fuzz_weird_types(tar_t* tar) {
    char bad_types[] = {0, 1, 127, 255, 'Z', '\0'};
    for(int i = 0; i < 6; i++) {
        init_valid_tar(tar);
        tar->typeflag = bad_types[i];
        run_test(tar, "", "", 0);
    }
}
int generate_inputs() {
    
    tar_t candidate = {0};
    // init_valid_tar(&candidate);
    // fuzz_size_with_empty_file(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_time(&candidate);
     
    //init_valid_tar(&candidate);
    //fuzz_magic_version(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_typeflag(&candidate);
    
    // init_valid_tar(&candidate);
    // fuzz_name(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_mode(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_guid(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_checksum(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_links(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_chains("");

    // init_valid_tar(&candidate);
    // fuzz_extreme_sizes(&candidate);


    // fuzz_null_archive();

    // init_valid_tar(&candidate);
    // fuzz_circular_links(&candidate);


    // init_valid_tar(&candidate);
    // fuzz_uname_gname(&candidate);


    // init_valid_tar(&candidate);
    // fuzz_extreme_octal_size(&candidate);

    // init_valid_tar(&candidate);
    // fuzz_metadata_overflow(&candidate);


    // init_valid_tar(&candidate);
    // fuzz_weird_types(&candidate);

    // fuzz_null_archive();

    return 0;
}

