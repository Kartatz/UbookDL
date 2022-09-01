#include <stdlib.h>
#include <stdio.h>

#include <curl/curl.h>
#include <json-c/json.h>
#include <json-c/json_util.h>

#include "query.h"
#include "errors.h"
#include "ssl.h"
#include "io.h"
#include "os.h"
#include "types.h"
#include "callbacks.h"

#define MAX_STRING_SIZE 1024
#define MAX_TOKEN_SIZE 36

static const char INVALID_FILENAME_CHARS[] = {
	' ', '/', '\\', ':', '*', '?', '\"', '<', '>', '|', '^', '\x00'
};

static const char TOKEN_FILENAME[] = "token.txt";

int main(const int argc, const char* argv[]) {
	
	#ifdef _WIN32
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
	#endif
	
	char username[MAX_STRING_SIZE + 1] = {'\0'};
	char password[MAX_STRING_SIZE + 1] = {'\0'};
	char url[MAX_STRING_SIZE + 1] = {'\0'};
	char access_token[MAX_TOKEN_SIZE + 1] = {'\0'};
	
	for (int index = 1; index < argc; ++index) {
		const char* argument_start = argv[index];
		const char* argument_end = strchr(argument_start, '\0');
		
		const char* key_end = strstr(argument_start, "=");
		
		if (key_end == NULL) {
			write_stderr("ubookdl: erro: O argumento '%s' não possui um valor definido", argument_start);
			return EXIT_FAILURE;
		}
		
		const size_t key_size = key_end - argument_start;
		
		if (key_size < 5) {
			write_stderr("ubookdl: erro: O argumento '%s' é inválido ou não foi reconhecido\x12\x01", argument_start);
			return EXIT_FAILURE;
		}
		
		char key[key_size];
		memcpy(key, argument_start, key_size);
		key[key_size] = '\0';
		
		key_end++;
		const size_t value_size = argument_end - key_end;
		
		if (value_size < 1) {
			write_stderr("ubookdl: erro: O argumento '%s' possui um valor inválido ou não reconhecido\r\n", key);
			return EXIT_FAILURE;
		}
		
		char value[value_size];
		memcpy(value, key_end, value_size);
		value[value_size] = '\0';
		
		if (strcmp(key, "--username") == 0) {
			strcpy(username, value);
		} else if (strcmp(key, "--password") == 0) {
			strcpy(password, value);
		} else if (strcmp(key, "--url") == 0) {
			strcpy(url, value);
		} else {
			write_stderr("ubookdl: erro: O argumento '%s' é inválido ou não foi reconhecido\r\n", key);
			return EXIT_FAILURE;
		}
	}
	
	if (file_exists(TOKEN_FILENAME) && get_file_size(TOKEN_FILENAME) == MAX_TOKEN_SIZE) {
		FILE* file = fopen(TOKEN_FILENAME, "r");
		
		if (file == NULL) {
			perror("fopen()");
			return EXIT_FAILURE;
		}
		
		fgets(access_token, sizeof(access_token), file);
		
		fclose(file);
	} else {
		if (*username == '\0') {
			while (1) {
				printf("> Usuário: ");
				
				if (fgets(username, sizeof(username), stdin) != NULL && *username != '\n') {
					break;
				}
				
				write_stderr("Nome de usuário inválido ou não reconhecido\r\n");
			}
			
			*strchr(username, '\n') = '\0';
		}
		
		if (*password == '\0') {
			while (1) {
				printf("> Senha: ");
				
				if (fgets(password, sizeof(), stdin) != NULL && *password != '\n') {
					break;
				}
				
				write_stderr("Senha inválida ou não reconhecida\r\n");
			}
			
			*strchr(password, '\n') = '\0';
		}
	}
	
	if (*url == '\0') {
		while (1) {
			printf("> URL: ");
			
			if (fgets(url, sizeof(url), stdin) != NULL && *url != '\n') {
				break;
			}
			
			write_stderr("URL inválida ou não reconhecida\r\n");
		}
		
		*strchr(url, '\n') = '\0';
	}
	
	CURLU* uri = curl_url();
	
	if (uri == NULL) {
		write_stderr("ubookdl: erro: não foi possível alocar memória\r\n");
		return EXIT_FAILURE;
	}
	puts(url);
	if (curl_url_set(uri, CURLUPART_URL, url, 0) != CURLE_OK) {
		write_stderr("ubookdl: erro: url inválida ou não reconhecida\r\n");
		puts("a");
		return EXIT_FAILURE;
	}
	
	char* path = NULL;
	
	if (curl_url_get(uri, CURLUPART_PATH, &path, 0) != CURLE_OK) {
		write_stderr("ubookdl: erro: url inválida ou; não reconhecida\r\n");
		puts("b");
		return EXIT_FAILURE;
	}
	
	if (strlen(path) < 2) {
		puts("c");
		write_stderr("ubookdl: erro: url inválida ou não reconhecida\r\n");
		return EXIT_FAILURE;
	}
	
	const char* start = strstr(path + 1, "/");
	start++;
	const char* end = strstr(start, "/");
	
	if (end == NULL) {
		end = strchr(path, '\0');
	}
	
	const size_t size = end - start;
	
	char book_id[size];
	memcpy(book_id, start, size);
	book_id[size] = '\0';
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	CURL* curl = curl_easy_init();
	
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Ubook HTTP Client");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_buffer_cb);
	
	#if defined(__unix__) && !defined(__ANDROID__) || __APPLE__
		curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
	#endif
	
	struct curl_blob blob = {
		.data = (char*) CACERT,
		.len = strlen(CACERT),
		.flags = CURL_BLOB_COPY
	};
	
	curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
	
	if (*access_token == '\0') {
		struct Query query = {};
		
		add_parameter(&query, "username", username);
		add_parameter(&query, "password", password);
		
		char* post_fields = NULL;
		
		if (query_stringify(query, &post_fields) == UBOOKDLERR_MEMORY_ALLOCATE_FAILURE) {
			write_stderr("ubookdl: erro: não foi possível alocar memória\r\n");
			return EXIT_FAILURE;
		}
		
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
		
		struct String string = {};
		
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
		curl_easy_setopt(curl, CURLOPT_URL, "https://vivo.ubook.com/backend/login");
		
		const CURLcode code = curl_easy_perform(curl);
		
		query_free(&query);
		
		if (code != CURLE_OK) {
			write_stderr("curl_easy_perform(): %s\r\n", curl_easy_strerror(code));
			return EXIT_FAILURE;
		}
		
		struct json_object* tree = json_tokener_parse(string.s);
		
		string_free(&string);
		
		if (tree == NULL) {
			write_stderr("json_tokener_parse(): could not parse json tree\r\n");
			return EXIT_FAILURE;
		}
		
		json_object* obj = json_object_object_get(tree, "success");
		
		if (!json_object_get_boolean(obj)) {
			write_stderr("ubookdl: erro: não foi possível realizar login\r\n");
			return EXIT_FAILURE;
		}
		
		printf("login realizado com sucesso!\r\n");
		
		json_object* info = json_object_object_get(tree, "data");
		json_object* token = json_object_object_get(info, "token");
		
		strcpy(access_token, json_object_get_string(token));
		
		FILE* file = fopen(TOKEN_FILENAME, "w");
		
		if (file == NULL) {
			perror("fopen()");
			return EXIT_FAILURE;
		}
		
		fwrite(access_token, strlen(access_token), 1, file);
		fclose(file);
	} else {
		struct Query query = {};
		
		add_parameter(&query, "token", access_token);
		
		char* post_fields = NULL;
		
		if (query_stringify(query, &post_fields) == UBOOKDLERR_MEMORY_ALLOCATE_FAILURE) {
			write_stderr("ubookdl: erro: não foi possível alocar memória\r\n");
			return EXIT_FAILURE;
		}
		
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
		
		struct String string = {};
		
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
		curl_easy_setopt(curl, CURLOPT_URL, "https://www.ubook.com/backend/pingUserSession");
		
		const CURLcode code = curl_easy_perform(curl);
		
		query_free(&query);
		
		if (code != CURLE_OK) {
			write_stderr("curl_easy_perform(): %s\r\n", curl_easy_strerror(code));
			return EXIT_FAILURE;
		}
		
		struct json_object* tree = json_tokener_parse(string.s);
		
		string_free(&string);
		
		if (tree == NULL) {
			write_stderr("json_tokener_parse(): could not parse json tree\r\n");
			return EXIT_FAILURE;
		}
		
		json_object* obj = json_object_object_get(tree, "success");
		
		const int success = obj != NULL && json_object_get_boolean(obj);
		
		json_object_put(tree);
		
		if (!success) {
			remove_file(TOKEN_FILENAME);
			
			write_stderr("ubookdl: erro: chave de acesso inválida ou expirada, realize o login novamente\r\n");
			return EXIT_FAILURE;
		}
	}
	
	struct Query query = {};
	
	add_parameter(&query, "id", book_id);
	
	char* post_fields = NULL;
	
	if (query_stringify(query, &post_fields) == UBOOKDLERR_MEMORY_ALLOCATE_FAILURE) {
		write_stderr("ubookdl: erro: não foi possível alocar memória\r\n");
		return EXIT_FAILURE;
	}
	
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
	
	struct String string = {};
	
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
	curl_easy_setopt(curl, CURLOPT_URL, "https://www.ubook.com/backend/product");
	
	CURLcode code = curl_easy_perform(curl);
	
	query_free(&query);
	
	if (code != CURLE_OK) {
		write_stderr("curl_easy_perform(): %s\r\n", curl_easy_strerror(code));
		return EXIT_FAILURE;
	}
	
	struct json_object* tree = json_tokener_parse(string.s);
	
	string_free(&string);
	
	if (tree == NULL) {
		write_stderr("json_tokener_parse(): could not parse json tree");
		return EXIT_FAILURE;
	}
	
	json_object* obj = json_object_object_get(tree, "success");
	
	if (!json_object_get_boolean(obj)) {
		write_stderr("ubookdl: erro: não foi possível obter informações sobre o livro\r\n");
		return EXIT_FAILURE;
	}
	
	json_object* info = json_object_object_get(tree, "data");
	json_object* product = json_object_object_get(info, "product");
	
	const char* title = json_object_get_string(json_object_object_get(product, "title"));
	const char* engine = json_object_get_string(json_object_object_get(product, "engine"));
	
	const char* endpoint = NULL;
	const char* document_file = NULL;
	
	char filename[strlen(title) + 4 + 1];
	strcpy(filename, title);
	
	char* ptr = strpbrk(filename, INVALID_FILENAME_CHARS);
	
	while (ptr != NULL) {
		*ptr = '_';
		ptr = strpbrk(ptr, INVALID_FILENAME_CHARS);
	}
	
	if (strcmp(engine, "ebook-epub") == 0) {
		endpoint = "https://www.ubook.com/backend/getEpubFile";
		document_file = "epub_file";
		strcat(filename, ".epub");
	} else if (strcmp(engine, "ebook-pdf") == 0) {
		endpoint = "https://www.ubook.com/backend/getPDFFile";
		document_file = "pdf_file";
		strcat(filename, ".pdf");
	} else {
		write_stderr("ubookdl: erro: o ubookdl ainda não suporta esse tipo de livro :(\r\n");
		return EXIT_FAILURE;
	}
	
	FILE* file = fopen(filename, "wb");
	
	if (file == NULL) {
		perror("fopen()");
		return EXIT_FAILURE;
	}
	
	char* fullpath = NULL;
	expand_filename(filename, &fullpath);
	
	add_parameter(&query, "id", book_id);
	add_parameter(&query, "token", access_token);
	
	post_fields = NULL;
	
	if (query_stringify(query, &post_fields) == UBOOKDLERR_MEMORY_ALLOCATE_FAILURE) {
		write_stderr("ubookdl: erro: não foi possível alocar memória\r\n");
		return EXIT_FAILURE;
	}
		
	curl_easy_setopt(curl, CURLOPT_URL, endpoint);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
	
	code = curl_easy_perform(curl);
	
	query_free(&query);
	
	if (code != CURLE_OK) {
		write_stderr("curl_easy_perform(): %s\r\n", curl_easy_strerror(code));
		return EXIT_FAILURE;
	}
	
	tree = json_tokener_parse(string.s);
	string_free(&string);
	
	if (tree == NULL) {
		write_stderr("json_tokener_parse(): could not parse json tree");
		return EXIT_FAILURE;
	}
	
	const char* download_url = json_object_get_string(json_object_object_get(json_object_object_get(tree, "data"), document_file));
	
	curl_easy_setopt(curl, CURLOPT_URL, download_url);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
	
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	
	printf("Baixando de %s para %s\r\n", download_url, fullpath);
	free(fullpath);
	
	code = curl_easy_perform(curl);
	
	fclose(file);
	
	if (code != CURLE_OK) {
		write_stderr("curl_easy_perform(): %s\r\n", curl_easy_strerror(code));
		return EXIT_FAILURE;
	}
	
	
	getchar();
	
	return EXIT_SUCCESS;
	
}