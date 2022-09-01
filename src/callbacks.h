#include <stdlib.h>

#include "types.h"

size_t curl_write_buffer_cb(char *chunk, size_t size, size_t nmemb, struct String* string);
size_t curl_write_file_cb(char *chunk, size_t size, size_t nmemb, FILE* stream);

#pragma once