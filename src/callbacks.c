#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"
#include "io.h"

size_t curl_write_buffer_cb(char *chunk, size_t size, size_t nmemb, struct String* string) {
	
	const size_t chunk_size = size * nmemb;
	
	const size_t slength = string->slength + chunk_size;
	
	string->s = realloc(string->s, slength + 1);
	
	if (string->s == NULL) {
		write_stderr("curl_write_buffer_cb(): cannot allocate memory\r\n");
		string_free(string);
		
		exit(EXIT_FAILURE);
	}
	
	memcpy(string->s + string->slength, chunk, chunk_size);
	
	string->s[slength] = '\0';
	string->slength = slength;
	
	return chunk_size;
	
}

size_t curl_write_file_cb(char *chunk, size_t size, size_t nmemb, FILE* stream) {
	return fwrite(chunk, size, nmemb, stream);
}