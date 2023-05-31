#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

#define MAX_WRITE_BUFFER_SIZE 11
#define MAX_READ_BUFFER_SIZE 20
#define SOCKET_PATH "/tmp/socket123"

void print_usage(char *argv0){
    printf("Usage: %s delay_in_seconds\n", argv0);
}

int main(int argc, char * argv[]) {
    int sockfd;
    struct sockaddr_un server_addr;
    char write_buffer[MAX_WRITE_BUFFER_SIZE];
    char read_buffer[MAX_READ_BUFFER_SIZE];
    char log_buffer[50];
    int bytes_received;
    double delay;
    struct timeval tv;
    int delay_count = 0;

    if (argc != 2){
        print_usage(argv[0]);
        exit(1);
    } else {
        delay = strtod(argv[1], NULL);
        tv.tv_sec = (int)delay;
        tv.tv_usec = (delay - tv.tv_sec) * 1000000;
    }
    
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("An error occured while creating a socket");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("An error occured while connecting to the socket");
        close(sockfd);
        exit(1);
    }

    srand(time(NULL));
    
    int read_bytes;
    int received_bytes;
    int next_write_index = 0;
    int bytes_left_to_delay = rand() % 255 + 1;
    do {
        read_bytes = read(STDIN_FILENO, write_buffer + next_write_index, 1);
        if (--bytes_left_to_delay == 0){
            if (delay != 0){
                struct timeval tv_c = tv;
                select(0, NULL, NULL, NULL, &tv_c);
                delay_count++;
            }
            bytes_left_to_delay = rand() % 255 + 1;
        }
        if (read_bytes == -1){
            perror("An error occured while reading from stdin");
            break;
        }
        if (read_bytes == 1){
            if (write_buffer[next_write_index++] == '\n'){
                if (write(sockfd, write_buffer, next_write_index) == -1) {
                    perror("An error occured while writing to socket");
                    break;
                }
                next_write_index = 0;
                received_bytes = read(sockfd, read_buffer, sizeof(read_buffer));
                if (received_bytes == -1) {
                    perror("An error occured while reading from socket");
                    break;
                } else if (received_bytes == 0) {
                    printf("Disconnected from the server");
                    break;
                } else {
                    read_buffer[received_bytes - 1] = '\0';
                    int n = snprintf(log_buffer, sizeof(log_buffer), "Server sent %s\n", read_buffer);
                    write(STDOUT_FILENO, log_buffer, n);
                }
            } else if (next_write_index == MAX_WRITE_BUFFER_SIZE){
                printf("Error: too long line from stdin");
                break;
            }
        }
    }
    while (read_bytes != 0);

    int n = snprintf(log_buffer, sizeof(log_buffer), "Total delay is %lf seconds\n", delay_count * delay);
    write(STDOUT_FILENO, log_buffer, n);

    close(sockfd);
    
    return 0;
}