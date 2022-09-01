int file_exists(const char* filename);
size_t get_file_size(const char* filename);
int remove_file(const char* filename);
int expand_filename(const char* filename, char** fullpath);

#pragma once