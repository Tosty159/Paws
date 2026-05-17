#include <stdio.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s file.paws\n", argv[0]);
		return 1;
	}

	char *filename = argv[1];
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "Error opening %s\n", filename);

		return 1;
	}

	return 0;
}