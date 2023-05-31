#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <poll.h>
#include <time.h>

#define SOCKET_PATH "/tmp/socket123"
#define MAX_CLIENTS_COUNT 100
#define MAX_BUFFER_SIZE 11

struct buffer_info{
    char buffer[MAX_BUFFER_SIZE];
    int next_write_index;
};

struct buffer_info CLIENTS_BUFFERS[MAX_CLIENTS_COUNT];

volatile bool is_running = true;

void set_is_running_false(){
    is_running = false;
}

int main()
{
    int server_fd, client_fds[MAX_CLIENTS_COUNT];
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    long state = 0;
    char log_buf[100];
    char long_str_buf[21];
    struct timespec start, end;
    void * void_ptr;
    int n;
    start.tv_sec = 0;
    start.tv_nsec = 0;
    end = start;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("An error occured while creating a socket");
        exit(1);
    }

    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("An error occured while fcntl");
        exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(server_fd, F_SETFL, flags) == -1) {
        perror("An error occured while fcntl");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("An error occured while binding to the socket");
        exit(1);
    }

    if (listen(server_fd, MAX_CLIENTS_COUNT) == -1) {
        perror("An error occured while listening to the socket");
        exit(1);
    }

    for (int i = 0; i < MAX_CLIENTS_COUNT; i++) {
        client_fds[i] = -1;
    }

    if (signal(SIGHUP, set_is_running_false) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit(1);
    }

    for (int i = 0; i < MAX_CLIENTS_COUNT; ++i){
        CLIENTS_BUFFERS[i].next_write_index = 0;
    }

    while (is_running) {
        struct pollfd fds[MAX_CLIENTS_COUNT + 1];
        fds[0].fd = server_fd;
        fds[0].events = POLLIN;
        for (int i = 0; i < MAX_CLIENTS_COUNT; i++) {
            if (client_fds[i] != -1) {
                fds[i + 1].fd = client_fds[i];
                fds[i + 1].events = POLLIN | POLLERR | POLLHUP;
            }
        }

        if (poll(fds, MAX_CLIENTS_COUNT + 1, -1) == -1) {
            perror("An error occured while polling");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
            if (client_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    perror("An error occured while accepting a socket");
                    continue;
                }
            } else {
                int i;
                for (i = 0; i < MAX_CLIENTS_COUNT; i++) {
                    if (client_fds[i] == -1) {
                        client_fds[i] = client_fd;
                        break;
                    }
                }
                if (i == MAX_CLIENTS_COUNT) {
                    fprintf(stderr, "The maximum number of clients is reached\n");
                    close(client_fd);
                } else {
                    void_ptr = sbrk(0);
                    n = snprintf(log_buf, sizeof(log_buf), "New client connected on fd %d, sbrk=%p\n", client_fd, &void_ptr);
                    write(STDOUT_FILENO, log_buf, n);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS_COUNT; i++) {
            if (client_fds[i] != -1 && fds[i + 1].revents & POLLIN) {
                int read_bytes = read(client_fds[i], 
                                    CLIENTS_BUFFERS[i].buffer + CLIENTS_BUFFERS[i].next_write_index, 
                                    MAX_BUFFER_SIZE - CLIENTS_BUFFERS[i].next_write_index);
                clock_gettime(CLOCK_MONOTONIC, &end);
                if (start.tv_sec == 0 && start.tv_nsec == 0) start = end;
                if (read_bytes == -1){
                    if (errno == EWOULDBLOCK){
                        continue;
                    }
                    n = snprintf(log_buf, sizeof(log_buf), "Client read error on fd %d\n", client_fds[i]);
                    write(STDOUT_FILENO, log_buf, n);
                    CLIENTS_BUFFERS[i].next_write_index = 0;
                    close(client_fds[i]);
                    client_fds[i] = -1;
                }
                else if (read_bytes == 0) {
                    n = snprintf(log_buf, sizeof(log_buf), "Client disconnected on fd %d\n", client_fds[i]);
                    write(STDOUT_FILENO, log_buf, strlen(log_buf));
                    CLIENTS_BUFFERS[i].next_write_index = 0;
                    close(client_fds[i]);
                    client_fds[i] = -1;
                } else {
                    CLIENTS_BUFFERS[i].next_write_index += read_bytes;
                    int j;
                    while (true){
                        for (j = 0; j < CLIENTS_BUFFERS[i].next_write_index; ++j){
                            if (CLIENTS_BUFFERS[i].buffer[j] == '\n'){
                                break;
                            }
                        }
                        if (j == CLIENTS_BUFFERS[i].next_write_index){
                            if (j == MAX_BUFFER_SIZE){
                                n = snprintf(log_buf, sizeof(log_buf), "Client on %d fd sent more than %d bytes in one line.\n", client_fds[i], MAX_BUFFER_SIZE - 1);
                                write(STDOUT_FILENO, log_buf, n);
                                CLIENTS_BUFFERS[i].next_write_index = 0;
                                close(client_fds[i]);
                                client_fds[i] = -1;
                            }
                            break;
                        }
                        else {
                            if (j == 0){
                                n = snprintf(log_buf, sizeof(log_buf), "Client on fd %d sent an empty line.\n", client_fds[i]);
                                write(STDOUT_FILENO, log_buf, n);
                                CLIENTS_BUFFERS[i].next_write_index = 0;
                                close(client_fds[i]);
                                client_fds[i] = -1;
                                continue;;
                            }
                            char * ptr;
                            long l = strtol(CLIENTS_BUFFERS[i].buffer, &ptr, 10);
                            if (CLIENTS_BUFFERS[i].buffer == ptr){
                                n = snprintf(log_buf, sizeof(log_buf), "Client on fd %d sent non-numeric data.\n", client_fds[i]);
                                write(STDOUT_FILENO, log_buf, n);
                                CLIENTS_BUFFERS[i].next_write_index = 0;
                                close(client_fds[i]);
                                client_fds[i] = -1;
                                continue;;
                            }

                            CLIENTS_BUFFERS[i].buffer[j] = '\0';
                            n = snprintf(log_buf, sizeof(log_buf), "Received %s from the client on fd %d\n", CLIENTS_BUFFERS[i].buffer, client_fds[i]);
                            write(STDOUT_FILENO, log_buf, n);
                            
                            CLIENTS_BUFFERS[i].next_write_index -= j + 1;
                            for (int k = 0; k < CLIENTS_BUFFERS[i].next_write_index; ++k){
                                CLIENTS_BUFFERS[i].buffer[k] = CLIENTS_BUFFERS[i].buffer[k + j + 1];
                            }
                            state += l;
                            n = snprintf(long_str_buf, sizeof(long_str_buf), "%ld\n", state);
                            if (write(client_fds[i], long_str_buf, n) == -1){

                            };
                            
                            long_str_buf[n - 1] = '\0';
                            n = snprintf(log_buf, sizeof(log_buf), "Sent %s to the client on fd %d\n", long_str_buf, client_fds[i]);
                            write(STDOUT_FILENO, log_buf, n);
                        } 
                    }
                }
            } else if (client_fds[i] != -1 && (fds[i + 1].revents & POLLERR || fds[i + 1].revents & POLLHUP)){
                snprintf(log_buf, sizeof(log_buf), "Client error on fd %d\n", client_fds[i]);
                write(STDOUT_FILENO, log_buf, strlen(log_buf));
                CLIENTS_BUFFERS[i].next_write_index = 0;
                close(client_fds[i]);
                client_fds[i] = -1;
            }
        }
    }

    snprintf(log_buf, sizeof(log_buf), "Seconds passed between the first and the last read: %lf\n", (double)( end.tv_sec - start.tv_sec ) + (double)( end.tv_nsec - start.tv_nsec ) / 1000000000);
    write(STDOUT_FILENO, log_buf, strlen(log_buf));

    for (int i = 0; i < MAX_CLIENTS_COUNT; i++) {
        if (client_fds[i] != -1){
            close(client_fds[i]);
        }
    }

    unlink(SOCKET_PATH);

    close(server_fd);

    exit(0);
}
