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
  vm the_vm;
  UT_hash_handle hh;
} tasks_waiting_on_exec;

static tasks_waiting_on_exec *exec_waiters = NULL;

static task_enum_action
exec_waiter_enumerator(task_closure closure, void *data)
{
  tasks_waiting_on_exec *tw, *tmp;

  HASH_FOREACH_SAFE(hh, exec_waiters, tw, tmp) {
    const char *status = "running";
    task_enum_action tea = (*closure)(tw->the_vm, status, data);
    if (tea == TEA_KILL) {
      HASH_DELETE(hh, exec_waiters, tw);
      myfree(tw, M_TASK);
    }
    if (tea != TEA_CONTINUE)
      return tea;
  }
  return TEA_CONTINUE;
}

static enum error
exec_waiter_suspender(vm the_vm, void *data)
{
    tasks_waiting_on_exec *tw = data;
    tw->the_vm = the_vm;
    HASH_ADD(hh, exec_waiters, p, sizeof(pid_t), tw);
    return E_NONE;
}

static package
bf_exec(Var arglist, Byte next, void *vdata, Objid progr)
{
  int i;

  for (i = 1; i <= arglist.v.list[0].v.num; i++) {
    if (arglist.v.list[i].type != TYPE_STR) {
      free_var(arglist);
      return make_error_pack(E_INVARG);
    }
  }
  if (1 == i) {
    free_var(arglist);
    return make_error_pack(E_ARGS);
  }

  const char *cmd = arglist.v.list[1].v.str;

  if (1 < strlen(cmd) && '.' == cmd[0] && '.' == cmd[1]) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid path", zero);
  }
  if (strstr(cmd, "/.")) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid path", zero);
  }

  static Stream *s;

  if(!s)
    s = new_stream(0);
  stream_add_string(s, BIN_SUBDIR);
  if('/' == cmd[0])
    stream_add_string(s, cmd + 1);
  else
    stream_add_string(s, cmd);
  
  cmd = reset_stream(s);

  struct stat buf;

  if(stat(cmd, &buf) != 0) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Does not exist", zero);
  }

  const char *args[i];
  
  for (i = 1; i <= arglist.v.list[0].v.num; i++) {
    args[i - 1] = arglist.v.list[i].v.str;
  }

  args[i - 1] = NULL;
  args[0] = cmd;

  pid_t p = fork();

  if (p == -1) {
    /* fork failed */
  }
  else if (0 == p) {
    /* child */
    static char *env[] = { "PATH=/bin:/usr/bin", NULL };
    int res;
    res = execve(cmd, (char *const *)args, (char *const *)env);
    oklog("EXEC: Executing %s failed with error code %d...\n", cmd, res);
    exit(res);
  }
  else {
    /* parent */
    oklog("EXEC: Executing %s ...\n", cmd);
    free_var(arglist);
    tasks_waiting_on_exec *tw = mymalloc(sizeof(tasks_waiting_on_exec), M_TASK);
    tw->p = p;
    return make_suspend_pack(exec_waiter_suspender, tw);
  }

  free_var(arglist);
  return no_var_pack();
}

pid_t
child_completed(pid_t p)
{
  tasks_waiting_on_exec *tw;

  HASH_FIND(hh, exec_waiters, &p, sizeof(pid_t), tw);
  if (tw) {
    Var v;
    v.type = TYPE_INT;
    v.v.num = (int)p;

    resume_task(tw->the_vm, v);

    HASH_DELETE(hh, exec_waiters, tw);
    myfree(tw, M_TASK);

    return p;
  }

  return 0;
}

void
register_exec(void)
{
  register_task_queue(exec_waiter_enumerator);
  register_function("exec", 0, -1, bf_exec);
}
