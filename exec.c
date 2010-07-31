#include <sys/stat.h>

#include "my-string.h"
#include "my-unistd.h"

#include "functions.h"
#include "structures.h"
#include "streams.h"
#include "utils.h"

#include "exec.h"

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
    execve(cmd, (char *const *)args, (char *const *)env);
    /* should never reach here */
  }
  else {
    /* parent */
    oklog("EXEC: Executing %s ...\n", cmd);
    Var v;
    v.type = TYPE_INT;
    v.v.num = (int)p;
    free_var(arglist);
    return make_var_pack(v);
  }

  free_var(arglist);
  return no_var_pack();
}

void
register_exec(void)
{
  register_function("exec", 0, -1, bf_exec);
}
