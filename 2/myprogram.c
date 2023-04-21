#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static void print_usage(char *argv0){
	printf("Usage: %s filename\n", argv0);
}

static void close_file(int fd){
	if (close(fd) == -1){
		fprintf(stderr, "ERROR: can't close the file: %s\n", strerror(errno));		
	}
}

int main(int argc, char* argv[])
{
    if (argc < 2){
        fprintf(stderr, "ERROR: missing a filename argument.\n");
        print_usage(argv[0]);
        return 1;
    } else if (argc > 2){
        fprintf(stderr, "ERROR: extra arguments.\n");
        print_usage(argv[0]);
        return 1;
    }
	int fd = -1;
    const char* filename = argv[1];
    const char* lock_extension = ".lck";
    char* lock_filename = malloc(strlen(filename)+strlen(lock_extension));
    if (lock_filename == NULL){
        fprintf(stderr, "ERROR: can't allocate memory for lock's filename: %s\n", strerror(errno));
        return 1;
    }
    strcpy(lock_filename, filename);
    strcat(lock_filename, lock_extension);
    while (fd == -1){
        fd = open(lock_filename, O_CREAT | O_RDWR | O_EXCL, 0500);
        if (fd == -1 && errno != EEXIST){
            fprintf(stderr, "ERROR: can't create a lock file: %s\n", strerror(errno));
            free(lock_filename);
            return 1;
        }
    }
    int pid = getpid();
    char pid_str[10];
    sprintf(pid_str, "%d", pid);
    if (write(fd, pid_str, strlen(pid_str)) == -1){
        fprintf(stderr, "ERROR: can't create a lock file: %s\n", strerror(errno));
        free(lock_filename);
        close_file(fd);
        return 1;
    }
    sleep(1);
    if (lseek(fd, 0, SEEK_SET) == -1){
        fprintf(stderr, "ERROR: can't seek in the lock file: %s\n", strerror(errno));
        free(lock_filename);
        close_file(fd);
        return 1;
    }
    size_t buffer_size = 11;
    char buffer[buffer_size];
    size_t read_bytes_in_buffer_count = 0;
    while (read_bytes_in_buffer_count < buffer_size){
        ssize_t read_bytes_count = read(fd, buffer + read_bytes_in_buffer_count, buffer_size - read_bytes_in_buffer_count);
        if (read_bytes_count == -1){
            fprintf(stderr, "ERROR: can't read the lock file: %s\n", strerror(errno));
            free(lock_filename);
            close_file(fd);
            return 1;
        }
        else if (read_bytes_count == 0)
            break;
        read_bytes_in_buffer_count += read_bytes_count;
    }
    if (read_bytes_in_buffer_count != strlen(pid_str) || strncmp(buffer, pid_str, read_bytes_in_buffer_count) != 0){
        fprintf(stderr, "ERROR: the lock file doesn't contain the pid of the program\n");
        free(lock_filename);
        close_file(fd);
        return 1;
    }
    if (remove(lock_filename) == -1){
        fprintf(stderr, "ERROR: can't remove the lock file: %s\n", strerror(errno));
        free(lock_filename);
        close_file(fd);
        return 1;
    };
    return 0;
}
