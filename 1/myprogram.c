#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

static void usage(char *argv0){
	printf("Usage: %s [-b block_size] (output_file | input_file output_file)\n", argv0);
	exit(1);
}

int main(int argc, char* argv[])
{
    FILE *input, *output;
    int buffer_size = 4096;
    char buffer[buffer_size];
	int opt = 1;

	while ((opt = getopt(argc, argv, "b:")) != -1) {
		switch (opt) {
			case 'b':
				buffer_size = atoi(optarg);
				break;
			default:
				usage(argv[0]);
		}
	}

	

	if (optind >= argc){
		fprintf(stderr, "ERROR: missing name arguments.\n");
		usage(argv[0]);
	} else if (optind == argc - 1){
		input = stdin;
		output = fopen(argv[optind], "w");
	} else if (optind == argc - 2){
		input = fopen(argv[optind], "r");
		output = fopen(argv[optind + 1], "w");
	} else {
		fprintf(stderr, "ERROR: extra name arguments.\n");
		usage(argv[0]);
	}

	bool is_zero_buffer = false;
	int read_bytes_total = 0;
    while (true){
    	int read_bytes_in_buffer_count = 0;
		while (read_bytes_in_buffer_count < buffer_size){
			int read_bytes_count = fread(buffer + read_bytes_in_buffer_count, 1, buffer_size - read_bytes_in_buffer_count, input);
	       	if (read_bytes_count == -1)
				return -1;
			else if (read_bytes_count == 0)
				break;
			read_bytes_in_buffer_count += read_bytes_count;
		}
		if (read_bytes_in_buffer_count == 0){
			break;
		}
		read_bytes_total += read_bytes_in_buffer_count;
		is_zero_buffer = true;
		for (int i = 0; i < read_bytes_in_buffer_count; ++i){
			if (buffer[i] != '\0'){
				is_zero_buffer = false;
				break;
			}
		}
		if (is_zero_buffer){
			fseek(output, read_bytes_in_buffer_count, SEEK_CUR);
		} else {
			fwrite(buffer, 1, read_bytes_in_buffer_count, output);
		}
    }
	ftruncate(fileno(output), read_bytes_total);
	if (input != stdin)
    	fclose(input);
    fclose(output);

    return 0;
}
