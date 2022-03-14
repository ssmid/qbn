
#ifndef QBN_PROCESS_H
#define QBN_PROCESS_H

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


extern char** environ;

typedef struct {
    pid_t pid;
    int pipe_in[2];
    int pipe_out[2];
    int pipe_err[2];
    const char* command;
} UtilProcess;

UtilProcess* util_process_new(const char* command) {
    UtilProcess* p = (UtilProcess*) malloc(sizeof(UtilProcess));
    p->pid = 0;
    pipe(p->pipe_in);
    pipe(p->pipe_out);
    pipe(p->pipe_err);
    p->command = command;
    return p;
}

void util_process_run(UtilProcess* process) {
    process->pid = fork();
    if (!process->pid) {
        // child
        // SIGTERM if parent dies
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        // TODO: close std ones?
        
        // set pipes
        dup2(process->pipe_in[0], STDIN_FILENO);
        dup2(process->pipe_out[1], STDOUT_FILENO);
        dup2(process->pipe_err[1], STDERR_FILENO);

        // close pipe fd's since the used ones are duplicated to std
        close(process->pipe_in[0]);
        close(process->pipe_in[1]);
        close(process->pipe_out[0]);
        close(process->pipe_out[1]);
        close(process->pipe_err[0]);
        close(process->pipe_err[1]);

        const char** args = malloc(sizeof(char*) * 4);
        args[0] = "sh";
        args[1] = "-c";
        args[2] = process->command;
        args[3] = NULL;

        execve("/bin/sh", (char* const*) args, environ);
        // execv returns only on error
        int exec_errno = errno;
        fprintf(stderr, "Error executing command: %d\n", exec_errno);
        exit(1);
    }

    // close unused pipe ends on parent side
    close(process->pipe_in[0]);
    close(process->pipe_out[1]);
    close(process->pipe_err[1]);
}

size_t util_process_write(UtilProcess* process, const char* s) {
    size_t pos = 0;
    size_t s_len = strlen(s);
    while (pos < s_len) {
        size_t count = write(process->pipe_in[1], s+pos, s_len-pos);
        if (count == -1) {
            fprintf(stderr, "Error on write");
            return -1;
        }
        pos += count;
    }
    return pos;
}

size_t util_process_read_internal(int file_descr, char* buffer, size_t buffer_size) {
    size_t count = read(file_descr, buffer, buffer_size - 1);
    if (count == -1) {
        fprintf(stderr, "Error on read");
        return -1;
    }
    buffer[count] = 0;
    return count;
}

size_t util_process_read(UtilProcess* process, char* buffer, size_t buffer_size) {
    return util_process_read_internal(process->pipe_out[0], buffer, buffer_size);
}

size_t util_process_read_err(UtilProcess* process, char* buffer, size_t buffer_size) {
    return util_process_read_internal(process->pipe_err[0], buffer, buffer_size);
}

int util_process_wait_exit(UtilProcess* process) {
    close(process->pipe_in[1]);
    int status;
    waitpid(process->pid, &status, 0);
    return WEXITSTATUS(status);
}

int util_process_kill(UtilProcess* process) {
    return kill(process->pid, SIGKILL);
}

#endif //QBN_PROCESS_H
