#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void usage(char *argv0){
	printf("Usage: %s [-b block_size] (output_file | input_file output_file)\n", argv0);
}

static bool is_numeric(const char *str)
{
    while(*str != '\0')
    {
        if(*str < '0' || *str > '9')
            return false;
        str++;
    }
    return true;
}



int main(int argc, char* argv[])
{
	FILE *input, *output;
	size_t buffer_size = 4096;
	char buffer[buffer_size];
	int opt = 1;

	while ((opt = getopt(argc, argv, "b:")) != -1) {
		switch (opt) {
			case 'b':
				if (!is_numeric(optarg)){
					fprintf(stderr, "ERROR: the b parameter is not numeric.\n");
					usage(argv[0]);
				}
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
		if (output == NULL){
			fprintf(stderr, "ERROR: can't open the file %s for writing: %s\n", argv[optind], strerror(errno));
		}
	} else if (optind == argc - 2){
		input = fopen(argv[optind], "r");
		if (input == NULL){
			fprintf(stderr, "ERROR: can't open the file %s for reading: %s\n", argv[optind], strerror(errno));
		}
		output = fopen(argv[optind + 1], "w");
		if (output == NULL){
			fprintf(stderr, "ERROR: can't open the file %s for writing: %s\n", argv[optind + 1], strerror(errno));
		}
	} else {
		fprintf(stderr, "ERROR: extra name arguments.\n");
		usage(argv[0]);
	}

	if (input == NULL || output == NULL){
		usage(argv[0]);
	}

	bool is_zero_buffer = false;
	size_t read_bytes_total = 0;
    while (true){
    	ssize_t read_bytes_in_buffer_count = 0;
		while (read_bytes_in_buffer_count < buffer_size){
			ssize_t read_bytes_count = read(fileno(input),
					buffer + read_bytes_in_buffer_count, 
					buffer_size - read_bytes_in_buffer_count);
	       	if (read_bytes_count == -1){
				fprintf(stderr, "ERROR: can't read: %s\n", strerror(errno));
				exit(1);
			}
			else if (read_bytes_count == 0)
				break;
			read_bytes_in_buffer_count += read_bytes_count;
		}
		if (read_bytes_in_buffer_count == 0){
			break;
		}
		read_bytes_total += read_bytes_in_buffer_count;
		is_zero_buffer = true;
		for (size_t i = 0; i < read_bytes_in_buffer_count; ++i){
			if (buffer[i] != '\0'){
				is_zero_buffer = false;
				break;
			}
		}
		if (is_zero_buffer){
			if (lseek(fileno(output), read_bytes_in_buffer_count, SEEK_CUR) == -1){
				fprintf(stderr, "ERROR: can't seek: %s\n", strerror(errno));
				exit(1);
			}
		} else {
			if (write(fileno(output), buffer, read_bytes_in_buffer_count) == -1){
				fprintf(stderr, "ERROR: can't write: %s\n", strerror(errno));
				exit(1);
			}
		}
    }
	if (ftruncate(fileno(output), read_bytes_total) == -1){
		fprintf(stderr, "ERROR: can't truncate: %s\n", strerror(errno));
		exit(1);	
	}
    return 0;
}
