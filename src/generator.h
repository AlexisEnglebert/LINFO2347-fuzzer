#include "tar.h"
#pragma once

extern char* command;


int fuzz_mode(tar_t* a, const char* path);
int execute_extractor(const char* command_prefix);
int generate_inputs();
