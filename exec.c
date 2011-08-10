/******************************************************************************
  Copyright 2011 Todd Sundsted. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY TODD SUNDSTED ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
  EVENT SHALL TODD SUNDSTED OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  The views and conclusions contained in the software and documentation are
  those of the authors and should not be interpreted as representing official
  policies, either expressed or implied, of Todd Sundsted.
 *****************************************************************************/

#include <errno.h>
#include <sys/stat.h>

#include "my-fcntl.h"
#include "my-signal.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-unistd.h"

#include "exec.h"
#include "functions.h"
#include "list.h"
#include "storage.h"
#include "structures.h"
#include "streams.h"
#include "tasks.h"
#include "utils.h"

typedef enum {
    TWS_CONTINUE,		/* The task is running and has not yet
				 * stopped or been killed.
				 */
    TWS_STOP,			/* The task has stopped.  This status
				 * is final.
				 */
    TWS_KILL			/* The task has been killed.  This
				 * status is final.
				 */
} task_waiting_status;

typedef struct tasks_waiting_on_exec {
    char *cmd;
    pid_t pid;
    task_waiting_status status;
    int code;
    int in;
    int out;
    int err;
    Stream *sout;
    Stream *serr;
    vm the_vm;
} tasks_waiting_on_exec;

static tasks_waiting_on_exec *process_table[EXEC_MAX_PROCESSES];

volatile static sig_atomic_t sigchild_interrupt = 0;

static sigset_t block_sigchld;

#define BLOCK_SIGCHLD sigprocmask(SIG_BLOCK, &block_sigchld, NULL)
#define UNBLOCK_SIGCHLD sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL)

static tasks_waiting_on_exec *
malloc_tasks_waiting_on_exec()
{
    tasks_waiting_on_exec *tw =
	mymalloc(sizeof(tasks_waiting_on_exec), M_TASK);
    tw->cmd = NULL;
    tw->status = TWS_CONTINUE;
    tw->code = 0;
    tw->sout = new_stream(1000);
    tw->serr = new_stream(1000);
    return tw;
}

static void
free_tasks_waiting_on_exec(tasks_waiting_on_exec * tw)
{
    if (tw->cmd)
	free_str(tw->cmd);
    close(tw->out);
    close(tw->err);
    network_unregister_fd(tw->out);
    network_unregister_fd(tw->err);
    if (tw->sout)
	free_stream(tw->sout);
    if (tw->serr)
	free_stream(tw->serr);
    myfree(tw, M_TASK);
}

static task_enum_action
exec_waiter_enumerator(task_closure closure, void *data)
{
    task_waiting_status status;
    int i;
    for (i = 0; i < EXEC_MAX_PROCESSES; i++) {
	if (process_table[i]) {
	    BLOCK_SIGCHLD;
	    status = process_table[i]->status;
	    UNBLOCK_SIGCHLD;
	    if (TWS_KILL != status) {
		task_enum_action tea =
		    (*closure) (process_table[i]->the_vm,
				process_table[i]->cmd,
				data);
		if (TEA_KILL == tea) {
		    BLOCK_SIGCHLD;
		    process_table[i]->status = TWS_KILL;
		    UNBLOCK_SIGCHLD;
		}
		if (TEA_CONTINUE != tea)
		    return tea;
	    }
	}
    }
    return TEA_CONTINUE;
}

static int
write_all(int fd, const char *buffer, size_t length)
{
    ssize_t count;
    while (length) {
	if ((count = write(fd, buffer, length)) < 0)
	    return -1;
	buffer += count;
	length -= count;
    }
    fsync(fd);
    return 1;
}

static void
stdout_readable(int fd, void *data)
{
    tasks_waiting_on_exec *tw = data;
    char buffer[1000];
    int n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
	stream_add_string(tw->sout, raw_bytes_to_binary(buffer, n));
    }
}

static void
stderr_readable(int fd, void *data)
{
    tasks_waiting_on_exec *tw = data;
    char buffer[1000];
    int n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
	stream_add_string(tw->serr, raw_bytes_to_binary(buffer, n));
    }
}

static enum error
exec_waiter_suspender(vm the_vm, void *data)
{
    enum error err = E_QUOTA;
    tasks_waiting_on_exec *tw = data;
    network_register_fd(tw->out, stdout_readable, NULL, tw);
    network_register_fd(tw->err, stderr_readable, NULL, tw);
    tw->the_vm = the_vm;
    BLOCK_SIGCHLD;
    int i;
    for (i = 0; i < EXEC_MAX_PROCESSES; i++) {
	if (process_table[i] == NULL) {
	    process_table[i] = tw;
	    err = E_NONE;
	    break;
	}
    }
    UNBLOCK_SIGCHLD;
    return err;
}

static int
set_nonblocking(int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0
	|| fcntl(fd, F_SETFL, flags | NONBLOCK_FLAG) < 0)
	return 0;
    else
	return 1;
}

static package
bf_exec(Var arglist, Byte next, void *vdata, Objid progr)
{
    /* The first argument must be a list of strings.  The first string
     * is the command.  The rest are arguments to the command.
     */
    int i;
    for (i = 1; i <= arglist.v.list[1].v.list[0].v.num; i++) {
	if (arglist.v.list[1].v.list[i].type != TYPE_STR) {
	    free_var(arglist);
	    return make_error_pack(E_INVARG);
	}
    }
    if (1 == i) {
	free_var(arglist);
	return make_error_pack(E_ARGS);
    }
    const char *args[i];
    for (i = 1; i <= arglist.v.list[1].v.list[0].v.num; i++) {
	args[i - 1] = arglist.v.list[1].v.list[i].v.str;
    }
    args[i - 1] = NULL;

    if (!is_wizard(progr)) {
	free_var(arglist);
	return make_error_pack(E_PERM);
    }

    /* Check the path. */
    const char *cmd = args[0];
    if (0 == strlen(cmd)) {
	free_var(arglist);
	return make_raise_pack(E_INVARG, "Invalid path", zero);
    }
    if (('/' == cmd[0])
	|| (1 < strlen(cmd) && '.' == cmd[0] && '.' == cmd[1])) {
	free_var(arglist);
	return make_raise_pack(E_INVARG, "Invalid path", zero);
    }
    if (strstr(cmd, "/.") || strstr(cmd, "./")) {
	free_var(arglist);
	return make_raise_pack(E_INVARG, "Invalid path", zero);
    }

    /* Prepend the exec subdirectory path. */
    static Stream *s;
    if (!s)
	s = new_stream(0);
    stream_add_string(s, EXEC_SUBDIR);
    stream_add_string(s, cmd);
    cmd = reset_stream(s);

    /* Stat the command. */
    struct stat buf;
    if (stat(cmd, &buf) != 0) {
	free_var(arglist);
	return make_raise_pack(E_INVARG, "Does not exist", zero);
    }
    if (!S_ISREG(buf.st_mode)) {
	free_var(arglist);
	return make_raise_pack(E_INVARG, "Is not a file", zero);
    }

    /* Create pipes, fork, and exec. */
    pid_t pid;
    int pipeIn[2];
    int pipeOut[2];
    int pipeErr[2];

    if (pipe(pipeIn) < 0) {
	free_var(arglist);
	log_perror("EXEC: Couldn't create pipe - pipeIn");
	return make_raise_pack(E_EXEC, "Exec failed", zero);
    } else if (pipe(pipeOut) < 0) {
	close(pipeIn[0]);
	close(pipeIn[1]);
	free_var(arglist);
	log_perror("EXEC: Couldn't create pipe - pipeOut");
	return make_raise_pack(E_EXEC, "Exec failed", zero);
    } else if (pipe(pipeErr) < 0) {
	close(pipeIn[0]);
	close(pipeIn[1]);
	close(pipeOut[0]);
	close(pipeOut[1]);
	free_var(arglist);
	log_perror("EXEC: Couldn't create pipe - pipeErr");
	return make_raise_pack(E_EXEC, "Exec failed", zero);
    } else if ((pid = fork()) < 0) {
	close(pipeIn[0]);
	close(pipeIn[1]);
	close(pipeOut[0]);
	close(pipeOut[1]);
	close(pipeErr[0]);
	close(pipeErr[1]);
	free_var(arglist);
	log_perror("EXEC: Couldn't fork");
	return make_raise_pack(E_EXEC, "Exec failed", zero);
    } else if (0 == pid) {
	/* child */
	int status;
	if ((status = dup2(pipeIn[0], STDIN_FILENO)) < 0) {
	    perror("dup2");
	    exit(status);
	}
	if ((status = dup2(pipeOut[1], STDOUT_FILENO)) < 0) {
	    perror("dup2");
	    exit(status);
	}
	if ((status = dup2(pipeErr[1], STDERR_FILENO)) < 0) {
	    perror("dup2");
	    exit(status);
	}
	close(pipeIn[1]);
	close(pipeOut[0]);
	close(pipeErr[0]);
	static char *env[] = { "PATH=/bin:/usr/bin", NULL };
	int res = execve(cmd, (char *const *)args, (char *const *)env);
	perror("execve");
	exit(res);
    }

    package pack = no_var_pack();

    oklog("EXEC: %s (%d)...\n", cmd, pid);

    tasks_waiting_on_exec *tw = malloc_tasks_waiting_on_exec();

    tw->cmd = str_dup(cmd);
    tw->pid = pid;
    tw->in = pipeIn[1];
    tw->out = pipeOut[0];
    tw->err = pipeErr[0];

    set_nonblocking(tw->in);
    set_nonblocking(tw->out);
    set_nonblocking(tw->err);

    close(pipeIn[0]);
    close(pipeOut[1]);
    close(pipeErr[1]);

    if (arglist.v.list[0].v.num > 1) {
	int len;
	const char *in = binary_to_raw_bytes(arglist.v.list[2].v.str, &len);
	if (in == NULL) {
	    pack = make_error_pack(E_INVARG);
	    goto call_free_tasks_waiting_on_exec;
	}
	if (write_all(tw->in, in, len) < 0) {
	    pack = make_raise_pack(E_INVARG, str_dup(strerror(errno)), zero);
	    goto call_free_tasks_waiting_on_exec;
	}
    }

    pack = make_suspend_pack(exec_waiter_suspender, tw);
    goto done;

 call_free_tasks_waiting_on_exec:
    free_tasks_waiting_on_exec(tw);

 done:
    free_var(arglist);
    close(tw->in);

    return pack;
}

/*
 * Called from child_completed_signal() in server.c.
 * SIGCHLD is already blocked.
 */
pid_t
exec_complete(pid_t pid, int code)
{
    tasks_waiting_on_exec *tw = NULL;

    int i;
    for (i = 0; i < EXEC_MAX_PROCESSES; i++)
	if (process_table[i] && process_table[i]->pid == pid) {
	    tw = process_table[i];
	    break;
	}

    if (tw) {
	sigchild_interrupt = 1;

	if (TWS_CONTINUE == tw->status) {
	    tw->status = TWS_STOP;
	    tw->code = code;
	}

	return pid;
    }

    return 0;
}

/*
 * Called from main_loop() in server.c.
 */
void
deal_with_child_exit(void)
{
    if (!sigchild_interrupt)
	return;

    sigchild_interrupt = 0;

    tasks_waiting_on_exec *tw = NULL;

    BLOCK_SIGCHLD;

    int i;
    for (i = 0; i < EXEC_MAX_PROCESSES; i++) {
	tw = process_table[i];
	if (tw && TWS_STOP == tw->status) {
	    Var v;
	    v = new_list(3);
	    v.v.list[1].type = TYPE_INT;
	    v.v.list[1].v.num = tw->code;
	    stdout_readable(tw->out, tw);
	    v.v.list[2].type = TYPE_STR;
	    v.v.list[2].v.str = str_dup(reset_stream(tw->sout));
	    stderr_readable(tw->err, tw);
	    v.v.list[3].type = TYPE_STR;
	    v.v.list[3].v.str = str_dup(reset_stream(tw->serr));

	    resume_task(tw->the_vm, v);
	}
	if (tw && TWS_CONTINUE != tw->status) {
	    free_tasks_waiting_on_exec(tw);
	    process_table[i] = NULL;
	}
    }

    UNBLOCK_SIGCHLD;
}

void
register_exec(void)
{
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, SIGCHLD);

    register_task_queue(exec_waiter_enumerator);
    register_function("exec", 1, 2, bf_exec, TYPE_LIST, TYPE_STR);
}
