#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 256
#define MAX_SYMBOLS 1024

typedef struct symbol {
	char name[BUFFER_SIZE];
	int offset;
} symbol;

symbol table[MAX_SYMBOLS];
int current_offset = 0;
int n = 0;

int read_statement(FILE *fp, char *buffer) {
	if (!fp || !buffer) return -1;

	int inside_brackets = 0;

	int i = 0;
	int ch = EOF;
	while (i < BUFFER_SIZE - 1 && (ch = fgetc(fp)) != EOF) {
		if (ch == ';' && !inside_brackets) {
			buffer[i] = '\0';
			break;
		}

		if (ch == '{') inside_brackets = 1;
		if (ch == '}') inside_brackets = 0;

		buffer[i++] = (char)ch;
	}

	if (i == 0 && ch == EOF) return -1; // Empty file

	return 0;
}

int read_statement_str(char *s, char *buffer) {
	if (!s || !buffer) return -1;

	int inside_brackets = 0;
	int i = 0;
	
	while (i < BUFFER_SIZE - 1 && *s != '\0') {
		if (*s == ';' && !inside_brackets) {
			buffer[i] = '\0';
			break;
		}
		if (*s == '{') inside_brackets = 1;
		if (*s == '}') inside_brackets = 0;
		
		buffer[i++] = *s;
		s++;
	}
	
	buffer[i] = '\0';
	
	if (i == 0 && *s == '\0') return -1;
	return 0;
}

int read_alphanum(char *src, char *dest, size_t size) {
	if (!src || !dest) return -1;

	while (isalnum(*src)) {
		*dest = *src;
		src++;
		dest++;
	}

	return 0;
}

int read_until(char *src, char *dest, size_t size, char delimiter) {
	if (!src || !dest) return -1;
	
	int i = 0;
	while (src[i] != '\0' && src[i] != delimiter && i < (int)size - 1) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
	return i;
}

void trim_left(char **s) {
	if (!s || !*s) return;

	while (isspace(**s)) {
		(*s)++;
	}
}

void trim_right(char **s) {
	if (!s || !*s) return;

	char *end = *s + strlen(*s) - 1;
	while (end >= *s && isspace(*end)) {
		end--;
	}
	*(end + 1) = '\0';
}

void trim(char **s) {
	if (!s || !*s) return;

	trim_left(s);
	trim_right(s);
}

int is_keyword(char *s) {
	char *keywords[3] = {
		"let", "fun", "return"
	};

	for (int i = 0; i < 3; i++) {
		if (strcmp(s, keywords[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int shunting_yard(char *expression) { return 0; }

char *compile_statement(char *statement);

void compile_assignment(char *result, char *identifier, char *expression) {
	int offset = 0;
	for (int i = 0; i <= n; i++) {
		if (strcmp(table[i].name, identifier) == 0) {
			offset = table[i].offset;
			break;
		}
	}
	if (offset == 0) {
		result = NULL;
		return;
	}

	int value = shunting_yard(expression);
	snprintf(result, BUFFER_SIZE, "mov [rsp + %d], %d\n", current_offset - offset, value);
}

void compile_declaration(char *result, char *identifier) {
	strcpy(table[n].name, identifier);
	current_offset -= 4;
	table[n].offset = current_offset;
	n++;

	strcpy(result, "sub rsp, 4\n");
}

void compile_function_declaration(char *result, char *identifier, char *args, char *block) {
	snprintf(result, 2 * BUFFER_SIZE, "%s:\n", identifier);

	char *sttm = malloc(BUFFER_SIZE);
	if (!sttm) {
		result = NULL;
		return;
	}

	while (read_statement_str(block, sttm) >= 0) {
		trim(&sttm);

		char *compiled = compile_statement(sttm);
		if (!compiled) {
			result = NULL;
			return;
		}
		strcat(result, compiled);

		block += strlen(sttm) + 1;
		if (*block == ';') block++;

		memset(sttm, 0, BUFFER_SIZE);
	}
}

char *compile_statement(char *statement) {
	if (!statement) return NULL;

	char *alnum = malloc(BUFFER_SIZE);
	char *result = malloc(16 * BUFFER_SIZE);
	if (!alnum || !result) {
		free(alnum); free(result);
		return NULL;
	}
	
	memset(alnum, 0, BUFFER_SIZE);
	memset(result, 0, 16 * BUFFER_SIZE);

	// Read first alphanumeric word
	if (read_alphanum(statement, alnum, BUFFER_SIZE) < 0) {
		free(alnum); free(result);
		return NULL;
	}

	char *assign = strchr(statement, '=');

	char *rest = statement + strlen(alnum);
	trim(&rest);
	trim(&alnum);

	char *identifier = malloc(BUFFER_SIZE);
	memset(identifier, 0, BUFFER_SIZE);

	int kw = is_keyword(alnum);
	switch(kw) {
		case -1: // Assignment
			if (!assign) {
				free(alnum); free(result);
				return NULL;
			}
			char *expression = assign + 1;
			trim(&expression);

			compile_assignment(result, alnum, expression);
			break;
		case 0: // Declaration
			if (!assign) {
				strcpy(identifier, rest);
			} else {
				read_until(rest, identifier, BUFFER_SIZE, '=');
			}
			trim(&identifier);

			compile_declaration(result, identifier);

			if (assign) {
				char *expression = assign + 1;
				trim(&expression);

				char temp[BUFFER_SIZE];
				compile_assignment(temp, identifier, assign);

				strcat(result, temp);
			}
			break;
		case 1: // Function declaration
			char *args = strchr(rest, '(');
			char *args_end = strchr(rest, ')');
			char *block = strchr(rest, '{');
			char *block_end = strrchr(rest, '}');
			if (!args || !args_end || !block || !block_end) {
				free(alnum); free(result);
				return NULL;
			}

			args++;
			*args_end = '\0';
			block++;
			*block_end = '\0';

			read_until(rest, identifier, BUFFER_SIZE, '(');
			trim(&identifier);

			compile_function_declaration(result, identifier, args, block);
			break;
		case 2: // Return statement
			int value = shunting_yard(rest);
			snprintf(result, BUFFER_SIZE, "mov rax, %d\nret\n", value);
			break;
		default:
			// Shouldn't get here
	}
	free(alnum);

	return result;
}

void compile(FILE *source_file, FILE *output_file) {
	if (!source_file || !output_file) return;

	char *header_text = "section .text\nglobal _start\n\n_start:\n";
	fwrite(header_text, sizeof(char), strlen(header_text), output_file);

	memset(table, 0, sizeof(table));

	char *statement = malloc(BUFFER_SIZE * sizeof *statement);
	memset(statement, 0, BUFFER_SIZE);

	while (read_statement(source_file, statement) >= 0) {
		trim(&statement);
		char *res = compile_statement(statement);
		if (!res) {
			return;
		}
		fwrite(res, sizeof(char), strlen(res), output_file);

		memset(statement, 0, BUFFER_SIZE);
	}
}