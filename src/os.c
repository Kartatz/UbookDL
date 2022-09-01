#include <stdlib.h>

#if defined(__unix__) || __APPLE__
	#include <unistd.h>
	#include <sys/stat.h>
#elif defined(_WIN32)
	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
	#endif
	
	#define _WIN32_WINNT 0x0600
	
	#include <windows.h>
	#include <fileapi.h>
#else
	#error "unsupported OS"
#endif

int file_exists(const char* filename) {
	
	#ifdef _WIN32
		return (GetFileAttributesA(filename) & FILE_ATTRIBUTE_DIRECTORY == 0);
	#else
		struct stat st = {};
		return (stat(filename, &st) >= 0 && S_ISREG(st.st_mode));
	#endif
	
}

size_t get_file_size(const char* filename) {
	
	#ifdef _WIN32
		WIN32_FIND_DATA data = {};
		const HANDLE handle = FindFirstFile(filename, &data);
		
		if (handle == INVALID_HANDLE_VALUE) {
			return 0;
		}
		
		FindClose(handle);
		
		return (data.nFileSizeHigh * MAXDWORD) + data.nFileSizeLow;
	#else
		struct stat st = {};
		return (stat(filename, &st) >= 0 ? st.st_size : 0);
	#endif
	
}

int remove_file(const char* filename) {
	
	#ifdef _WIN32
		return DeleteFile(filename);
	#else
		return unlink(filename) == 0;
	#endif
	
}

int expand_filename(const char* filename, char** fullpath) {
	
	#ifdef _WIN32
		char path[MAX_PATH];
		const DWORD code = GetFullPathNameA(filename, sizeof(path), path, NULL);
		
		if (code == 0) {
			return code;
		}
		
		*fullpath = malloc(code + 1);
		
		if (*fullpath == NULL) {
			return 0
		}
		
		strcpy(*fullpath, path);
	#else
		*fullpath = realpath(filename, NULL);
		
		if (*fullpath == NULL) {
			return 0;
		}
	#endif
	
	return 1;
	
}
