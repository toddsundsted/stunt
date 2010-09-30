/******************************************************************************
  Copyright 2010 Todd Sundsted. All rights reserved.

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

#include <sys/stat.h>

#include "my-string.h"
#include "my-unistd.h"

#include "uthash.h"

#include "exec.h"
#include "functions.h"
#include "list.h"
#include "storage.h"
#include "structures.h"
#include "streams.h"
#include "tasks.h"
#include "utils.h"

typedef struct tasks_waiting_on_exec {
  pid_t p;
  int in;
  int out;
  int err;
  Stream *sout;
  Stream *serr;
  vm the_vm;
  UT_hash_handle hh;
} tasks_waiting_on_exec;

static tasks_waiting_on_exec *exec_waiters = NULL;

static tasks_waiting_on_exec *
malloc_tasks_waiting_on_exec()
{
  tasks_waiting_on_exec *tw = mymalloc(sizeof(tasks_waiting_on_exec), M_TASK);
  tw->sout = new_stream(1000);
  tw->serr = new_stream(1000);
  return tw;
}

static void
free_tasks_waiting_on_exec(tasks_waiting_on_exec *tw)
{
  free_stream(tw->sout);
  free_stream(tw->serr);
  /* close(tw->in); already closed */
  network_unregister_fd(tw->out);
  network_unregister_fd(tw->err);
  close(tw->out);
  close(tw->err);
  myfree(tw, M_TASK);
}

static task_enum_action
exec_waiter_enumerator(task_closure closure, void *data)
{
  tasks_waiting_on_exec *tw, *tmp;

  HASH_FOREACH_SAFE(hh, exec_waiters, tw, tmp) {
    const char *status = "running";
    task_enum_action tea = (*closure)(tw->the_vm, status, data);
    if (TEA_KILL == tea) {
      HASH_DELETE(hh, exec_waiters, tw);
      free_tasks_waiting_on_exec(tw);
    }
    if (TEA_CONTINUE != tea)
      return tea;
  }
  return TEA_CONTINUE;
}

static void
write_all(int fd, const char *buffer, size_t length)
{
  while (length) {
    ssize_t count = write(fd, buffer, length);
    buffer += count;
    length -= count;
  }
}

static void
stdout_readable(int fd, void *data)
{
  tasks_waiting_on_exec *tw = data;
  char buffer[1000];
  int n;
  while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
    buffer[n] = '\0';
    stream_add_string(tw->sout, raw_bytes_to_binary(buffer, strlen(buffer)));
  }
}

static void
stderr_readable(int fd, void *data)
{
  tasks_waiting_on_exec *tw = data;
  char buffer[1000];
  int n;
  while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
    buffer[n] = '\0';
    stream_add_string(tw->serr, raw_bytes_to_binary(buffer, strlen(buffer)));
  }
}

static enum error
exec_waiter_suspender(vm the_vm, void *data)
{
  tasks_waiting_on_exec *tw = data;
  network_register_fd(tw->out, stdout_readable, NULL, tw);
  network_register_fd(tw->err, stderr_readable, NULL, tw);
  tw->the_vm = the_vm;
  HASH_ADD(hh, exec_waiters, p, sizeof(pid_t), tw);
  return E_NONE;
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

  /* Check the path. */
  const char *cmd = args[0];
  if (0 == strlen(cmd)) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid path", zero);
  }
  if (('/' == cmd[0]) || (1 < strlen(cmd) && '.' == cmd[0] && '.' == cmd[1])) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid path", zero);
  }
  if (strstr(cmd, "/.") || strstr(cmd, "./")) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid path", zero);
  }

  /* Prepend the bin subdirectory path. */
  static Stream *s;
  if (!s)
    s = new_stream(0);
  stream_add_string(s, BIN_SUBDIR);
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
  pid_t p;
  int pipeIn[2];
  int pipeOut[2];
  int pipeErr[2];

  if (pipe(pipeIn) < 0) {
    free_var(arglist);
    log_perror("EXEC: Couldn't create pipe - pipeIn");
    return make_raise_pack(E_EXEC, "Exec failed", zero);
  }
  else if (pipe(pipeOut) < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    free_var(arglist);
    log_perror("EXEC: Couldn't create pipe - pipeOut");
    return make_raise_pack(E_EXEC, "Exec failed", zero);
  }
  else if (pipe(pipeErr) < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    free_var(arglist);
    log_perror("EXEC: Couldn't create pipe - pipeErr");
    return make_raise_pack(E_EXEC, "Exec failed", zero);
  }
  else if ((p = fork()) < 0) {
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    close(pipeErr[0]);
    close(pipeErr[1]);
    free_var(arglist);
    log_perror("EXEC: Couldn't fork");
    return make_raise_pack(E_EXEC, "Exec failed", zero);
  }
  else if (0 == p) {
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
  else {
    /* parent */
    oklog("EXEC: Executing %s...\n", cmd);
    tasks_waiting_on_exec *tw = malloc_tasks_waiting_on_exec();
    tw->p = p;
    tw->in = pipeIn[1];
    tw->out = pipeOut[0];
    tw->err = pipeErr[0];
    close(pipeIn[0]);
    close(pipeOut[1]);
    close(pipeErr[1]);
    if (arglist.v.list[0].v.num > 1) {
      int len;
      write_all(tw->in, binary_to_raw_bytes(arglist.v.list[2].v.str, &len), len);
    }
    close(tw->in);
    free_var(arglist);
    return make_suspend_pack(exec_waiter_suspender, tw);
  }

  free_var(arglist);
  return no_var_pack();
}

pid_t
exec_completed(pid_t p, int code)
{
  tasks_waiting_on_exec *tw;

  HASH_FIND(hh, exec_waiters, &p, sizeof(pid_t), tw);
  if (tw) {
    Var v;
    v = new_list(3);
    v.v.list[1].type = TYPE_INT;
    v.v.list[1].v.num = code;
    stdout_readable(tw->out, tw);
    v.v.list[2].type = TYPE_STR;
    v.v.list[2].v.str = str_dup(reset_stream(tw->sout));
    stderr_readable(tw->err, tw);
    v.v.list[3].type = TYPE_STR;
    v.v.list[3].v.str = str_dup(reset_stream(tw->serr));

    resume_task(tw->the_vm, v);

    HASH_DELETE(hh, exec_waiters, tw);
    free_tasks_waiting_on_exec(tw);

    return p;
  }

  return 0;
}

void
register_exec(void)
{
  register_task_queue(exec_waiter_enumerator);
  register_function("exec", 1, 2, bf_exec, TYPE_LIST, TYPE_STR);
}
