#include "tar.h"
#pragma once

int execute_extractor(const char* path);

int generate_inputs(const char* path);

int fuzz_mode(tar_t* a, const char* path);
