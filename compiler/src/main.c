#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "compiler.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s file.paws\n", argv[0]);
		return 1;
	}

	char *path = argv[1];

	char *dot = strrchr(path, '.');
	if (!dot || strcmp(dot, ".paws") != 0) {
		perror("Source file should have '.paws' extensions.");
		return 1;
	}

	FILE *source_file = fopen(path, "r");
	if (!source_file) {
		fprintf(stderr, "Error opening %s\n", path);
		return 1;
	}

	*dot = '\0';
	char *filename = basename(path);
	char *dir = dirname(path);

	size_t size = strlen(dir) + strlen(filename) + 2; // 2 for '/' and null terminator
	char output_path[size];
	snprintf(output_path, size, "%s/%s", dir, filename);

	FILE *output_file = fopen(output_path, "w");
	if (!output_file) {
		fprintf(stderr, "Error opening %s\n", output_path);
		return 1;
	}

	compile(source_file, output_file);

	fclose(source_file);
	fclose(output_file);

	return 0;
}