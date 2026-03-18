#include "tar.h"
#pragma once

extern char* command;


int fuzz_mode(tar_t* a);
int execute_extractor(const char* command_prefix);
int generate_inputs();
int run_test(tar_t* tar, const char* command_prefix, const char* content, size_t data_size, int compute_checksum);
