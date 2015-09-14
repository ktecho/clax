/*
 *  Copyright (C) 2015, Clarive Software, All Rights Reserved
 *
 *  This file is part of clax.
 *
 *  Clax is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Clax is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Clax.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/wait.h>

#endif

#include "popen2.h"

#define READ 0
#define WRITE 1

#if defined(_WIN32)

int popen2(const char *cmdline, popen2_t *child)
{
    return 0;
}

int pclose2(popen2_t *child)
{
    return 0;
}

#else

int popen2(const char *cmdline, popen2_t *child)
{
    pid_t pid;
    int pipe_stdin[2], pipe_stdout[2];

    if (pipe(pipe_stdin))
        return -1;
    if (pipe(pipe_stdout))
        return -1;

    pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0) {
        setpgid(0, 0);

        close(pipe_stdin[WRITE]);
        dup2(pipe_stdin[READ], READ);

        close(pipe_stdout[READ]);
        dup2(pipe_stdout[WRITE], WRITE);

        execl("/bin/sh", "sh", "-c", cmdline, NULL);
        exit(99);
    }

    close(pipe_stdin[READ]);
    close(pipe_stdout[WRITE]);

    child->pid = pid;
    child->in = pipe_stdin[WRITE];
    child->out = pipe_stdout[READ];

    fcntl(child->out, F_SETFL, (fcntl(child->out, F_GETFL, 0) | O_NONBLOCK));

    return 0;
}

int pclose2(popen2_t *child)
{
    pid_t pid;
    int pstat;

    do {
        pid = waitpid(child->pid, &pstat, 0);
    } while (pid == -1 && errno == EINTR);

    return (pid == -1 ? -1 : pstat / 256);
}

#endif
