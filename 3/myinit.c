#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h> 
#include <sys/wait.h> 

#define MAXPROC 256
#define MAXBUFFERLEN 4096

FILE *log_file_ptr;
char *log_file_path = "/tmp/my_init.log";
char *configuration_file_path;

struct child_configuration{
    const char* program_path;
    char* const* argv;
    const char* input_path;
    const char* output_path;
};

int children_count = 0;
int childlen_pids[MAXPROC];
struct child_configuration children_configurations[MAXPROC];
volatile bool should_start_new_children_loop = true;

void set_should_start_new_children_loop_true(){
    should_start_new_children_loop = true;
}

void print_usage(char *argv0){
    printf("Usage: %s configuration_abs_path\n", argv0);
}

bool is_path_absolute(char* path){
    return (path[0] == '/');
}

char** split_string(char* str, const char * delim) {
    char** tokens = malloc(sizeof(char*) * MAXBUFFERLEN);
    char* token;
    int i = 0;

    token = strtok(str, delim);
    while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }

    tokens[i] = NULL;

    return tokens;
}

void start_child(int child_idx){
    pid_t cpid = fork();

    switch (cpid) 
    { 
        case -1:
            printf("Fork failed; cpid == -1\n");
            break; 
        case 0:
            cpid = getpid();
            if ((log_file_ptr = fopen(log_file_path, "a+")) == NULL){
                printf("ERROR: can't open the log file in the child proccess with %d pid: %s\n", cpid, strerror(errno));
                exit(1);
            }
            if (freopen(children_configurations[child_idx].input_path, "r+", stdin) == NULL){
                fprintf(log_file_ptr, "ERROR: can't open the input file in the child process with %d pid: %s\n", cpid, strerror(errno));
                fflush(log_file_ptr);
                exit(1);
            };
            if (freopen(children_configurations[child_idx].output_path, "w+", stdout) == NULL){
                fprintf(log_file_ptr, "ERROR: can't open the output file in the child process with %d pid: %s\n", cpid, strerror(errno));
                fflush(log_file_ptr);
                exit(1);
            };
            if (execv(children_configurations[child_idx].program_path, children_configurations[child_idx].argv) == -1){
                fprintf(log_file_ptr, "ERROR: can't exec a child process with %d pid: %s\n", cpid, strerror(errno));
                fflush(log_file_ptr);
                exit(1);
            }
        default:
            childlen_pids[child_idx] = cpid;
            fprintf(log_file_ptr, "Created a child process with %d pid.\n", cpid);
            fflush(log_file_ptr);
    }
}

int get_pointer_length(char** ptr){
    for (int i = 0; i < MAXBUFFERLEN; ++i){
        if (ptr[i] == NULL) return i;
    }
}

void children_loop()
{ 
    int i, p, child_status, lines_count = 0, argc;
    children_count = 0;
    pid_t cpid;
    FILE* configuration_file_ptr;

    should_start_new_children_loop = false;

    if ((configuration_file_ptr = fopen(configuration_file_path, "rb")) == NULL){
       fprintf(log_file_ptr, "ERROR: can't open the configuration file %s: %s\n", configuration_file_path, strerror(errno));
       fflush(log_file_ptr);
       exit(1);
    }

    while (lines_count++ < MAXPROC){
        char* line = malloc(MAXBUFFERLEN);

        if (line == NULL){
            fprintf(log_file_ptr, "ERROR: can't allocate memory for configuration line: %s\n", strerror(errno));
            exit(1);
        }

        if (fgets(line, MAXBUFFERLEN, configuration_file_ptr) == NULL) break;

        p = strlen(line) - 1;
        if (line[p] == '\n')
            line[p] = '\0';

        char** args = split_string(line, " ");
        argc = get_pointer_length(args);
        if (argc < 3 || !is_path_absolute(args[0]) || !is_path_absolute(args[argc - 1]) || !is_path_absolute(args[argc - 2])){
            fprintf(log_file_ptr, "ERROR: can't read the %d configuration line: wrong format.\n", lines_count);
            fflush(log_file_ptr);
            exit(1);
        }
        children_configurations[children_count] = (struct child_configuration){args[0], args, args[argc - 2], args[argc - 1]};
        args[argc - 2] = NULL;
        children_configurations[children_count].argv = args;
        start_child(children_count++);
    }



    fclose(configuration_file_ptr);

    while (children_count && !should_start_new_children_loop)
    {
        cpid = waitpid(-1, &child_status, WNOHANG);
        if (cpid == -1){
            fprintf(log_file_ptr, "ERROR: can't waitpid: %s\n", strerror(errno));
            fflush(log_file_ptr);
        }
        if (should_start_new_children_loop) break;
        if (cpid > 0){
            for (p = 0; p < children_count; ++p)
            {
                if (childlen_pids[p] == cpid)
                {
                    if (WIFEXITED(child_status)){
                        fprintf(log_file_ptr, "The child process with %d pid was terminated normally.\n", cpid);
                        fflush(log_file_ptr);
                    }
                    if (WIFSIGNALED(child_status)){
                        fprintf(log_file_ptr, "The child process with %d pid was terminated by a signal.\n", cpid);
                        fflush(log_file_ptr);
                    }
                    childlen_pids[p] = 0;
                    start_child(p);
                }
            }
        }
    }

    for (int i = 0; i < children_count; ++i){
        if (childlen_pids[i] > 0){
            kill(childlen_pids[i], SIGKILL);
            fprintf(log_file_ptr, "A kill signal was sent to the child proccess with %d pid.\n", childlen_pids[i]);
            fflush(log_file_ptr);
        }
    }
}

int main(int argc, char* argv[])
{
    if (signal(SIGHUP, set_should_start_new_children_loop_true) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit(1);
    }

    if (argc != 2 || !is_path_absolute(argv[1])){
        print_usage(argv[0]);
        exit(1);
    }

    int fd;
    struct rlimit flim;
    if (getppid() != 1){
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        if (fork() != 0)
            exit(0);
        setsid();
    }

    getrlimit(RLIMIT_NOFILE, &flim);
    for (fd = 0; fd < flim.rlim_max; fd++)
        close(fd);

    chdir("/");

    if ((log_file_ptr = fopen(log_file_path, "a+")) == NULL){
       printf("ERROR: can't open the log file in the parent proccess: %s\n", log_file_path);
       exit(1);
    }

    configuration_file_path = argv[1];

    while (true) {
        if (should_start_new_children_loop){
            children_loop();
        }
    }
}