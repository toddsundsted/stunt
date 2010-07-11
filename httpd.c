#include "my-types.h"
#include "my-string.h"

#include "platform.h"
#include <microhttpd.h>

#include "config.h"
#include "functions.h"
#include "list.h"
#include "storage.h"
#include "structures.h"
#include "execute.h"
#include "tasks.h"
#include "utils.h"
#include "log.h"

#include "base64.h"

#include "httpd.h"

struct MHD_Daemon *mhd_daemon;

typedef unsigned32 Connid;

static Connid max_id = 0;

struct connection_info_struct
{
  Connid id;
  Objid player;
  char *request_method;
  char *request_uri;
  char *request_type;
  char *request_body;
  int response_code;
  char *response_type;
  char *response_body;
  int authenticated;
  struct connection_info_struct *prev;
  struct connection_info_struct *next;
};

static struct connection_info_struct con_info_head;

void remove_connection_info_struct(struct connection_info_struct *con_info)
{
  if (con_info) {
    con_info->prev->next = con_info->next;
    con_info->next->prev = con_info->prev;
    if (con_info->request_method) free(con_info->request_method);
    if (con_info->request_uri) free(con_info->request_uri);
    if (con_info->request_type) free(con_info->request_type);
    if (con_info->request_body) free(con_info->request_body);
    if (con_info->response_type) free(con_info->response_type);
    if (con_info->response_body) free(con_info->response_body);
    free(con_info);
  }
}

void add_connection_info_struct(struct connection_info_struct *con_info)
{
  if (con_info_head.prev == NULL || con_info_head.next == NULL) {
    con_info_head.prev = &con_info_head;
    con_info_head.next = &con_info_head;
  }
  if (con_info) {
    con_info->prev = con_info_head.prev;
    con_info->next = &con_info_head;
    con_info->prev->next = con_info;
    con_info->next->prev = con_info;
  }
}

struct connection_info_struct *new_connection_info_struct()
{
  struct connection_info_struct *con_info = malloc(sizeof(struct connection_info_struct));
  if (NULL == con_info) return NULL;
  con_info->id = ++max_id;
  con_info->player = NOTHING;
  con_info->request_method = NULL;
  con_info->request_uri = NULL;
  con_info->request_type = NULL;
  con_info->request_body = NULL;
  con_info->response_code = 0;
  con_info->response_type = NULL;
  con_info->response_body = NULL;
  con_info->authenticated = 0;
  con_info->prev = NULL;
  con_info->next = NULL;
  add_connection_info_struct(con_info);
  return con_info;
}

struct connection_info_struct *find_connection_info_struct(Connid id)
{
  if (con_info_head.prev == &con_info_head || con_info_head.next == &con_info_head)
    return NULL;

  struct connection_info_struct *p = con_info_head.next;

  while (p != &con_info_head) {
    if (p->id == id)
      return p;
    p = p->next;
  }

  return NULL;
}

int ask_for_authentication(struct MHD_Connection *connection, const char *realm)
{
  int ret;
  struct MHD_Response *response;
  char *headervalue;
  const char *strbase = "Basic realm=";

  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (!response)
    return MHD_NO;

  headervalue = malloc (strlen (strbase) + strlen (realm) + 1);
  if (!headervalue)
    return MHD_NO;

  strcpy (headervalue, strbase);
  strcat (headervalue, realm);

  ret = MHD_add_response_header (response, "WWW-Authenticate", headervalue);
  free (headervalue);
  if (!ret)
    {
      MHD_destroy_response (response);
      return MHD_NO;
    }

  ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);

  MHD_destroy_response (response);

  return ret;
}

int is_authenticated(struct MHD_Connection *connection, struct connection_info_struct *con_info)
{
  size_t dummy;
  const char *headervalue;
  const char *strbase = "Basic ";
  const unsigned char *decoded;
  char *separator;
  int authenticated = 0;

  if (con_info->authenticated)
    return 1;

  headervalue =
    MHD_lookup_connection_value (connection, MHD_HEADER_KIND,
                                 "Authorization");
  if (NULL == headervalue)
    return 0;
  if (0 != strncmp (headervalue, strbase, strlen (strbase)))
    return 0;

  decoded = base64_decode(headervalue + strlen(strbase), strlen(headervalue) - strlen(strbase), &dummy);

  if (NULL == decoded)
    return 0;

  separator = strchr(decoded, ':');

  if (0 == separator) {
    free((char *)decoded);
    return 0;
  }

  const unsigned char *username = decoded;
  const unsigned char *password = separator + 1;
  *separator = 0;

  Var args;
  Var result;
  enum outcome outcome;

  args = new_list(2);
  args.v.list[1].type = TYPE_STR;
  args.v.list[1].v.str = str_dup(username);
  args.v.list[2].type = TYPE_STR;
  args.v.list[2].v.str = str_dup(password);
  outcome = run_server_task(-1, SYSTEM_OBJECT, "authenticate", args, "", &result);

  if (outcome == OUTCOME_DONE && result.type == TYPE_OBJ) {
    con_info->player = result.v.obj;
    con_info->authenticated = 1;
    authenticated = 1;
  }

  free_var(result);

  free((char *)decoded);

  return authenticated;
}

static void
dir_free_callback (void *cls)
{
  oklog("DIR FREE CALLBACK!\n");
}

static int
dir_reader (void *cls, uint64_t pos, char *buf, int max)
{
  oklog("DIR READER!\n");

  struct connection_info_struct *con_info = (struct connection_info_struct *)cls;

  if (con_info->response_body) {
    printf("pos = %d\n", pos);
    printf("max = %d\n", max);
    char * foo = strncpy(buf, con_info->response_body + pos, max);
    int len = strlen(foo);
    printf("len = %d\n", len);
    return len ? len : -1;
  }
  else {
    return 0;
  }
}

static void
request_completed (void *cls,
		   struct MHD_Connection *connection,
		   void **ptr,
		   enum MHD_RequestTerminationCode toe)
{
  oklog("REQUEST COMPLETED!\n");

  remove_connection_info_struct(*ptr);
}

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  struct connection_info_struct *con_info;

  if (NULL == *ptr) {
    con_info = new_connection_info_struct();
    if (NULL == con_info) return MHD_NO;
    con_info->request_method = strdup(method);
    con_info->request_uri = strdup(url);
    *ptr = (void *)con_info;
    return MHD_YES;
  }

  con_info = *ptr;
      
  if (!is_authenticated (connection, con_info))
    return ask_for_authentication (connection, "Moo");

  if (*upload_data_size != 0)
    {
      if (con_info->request_body)
	{
	  char *oldstring = con_info->request_body;
	  size_t len = strlen(oldstring) + *upload_data_size + 1;
	  con_info->request_body = malloc(len);
	  con_info->request_body[0] = 0;
	  strcat(con_info->request_body, oldstring);
	  strncat(con_info->request_body, upload_data, *upload_data_size);
	  con_info->request_body[len - 1] = 0;
	  free (oldstring);
	}
      else
	{
	  size_t len = *upload_data_size + 1;
	  con_info->request_body = malloc(len);
	  con_info->request_body[0] = 0;
	  strncat(con_info->request_body, upload_data, *upload_data_size);
	  con_info->request_body[len - 1] = 0;
	}

      *upload_data_size = 0;
      return MHD_YES;
    }

  const char *content_type;

  content_type = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, "Content-Type");

  con_info->request_type = content_type ? strdup(content_type) : strdup("");

  Var args;
  Var result;
  enum outcome outcome;
  int ret;

  args = new_list(1);
  args.v.list[1].type = TYPE_INT;
  args.v.list[1].v.num = con_info->id;
  outcome = run_server_task(con_info->player, SYSTEM_OBJECT, "do_httpd", args, "", &result);

  //  if (outcome == OUTCOME_DONE || outcome == OUTCOME_ABORTED)
  //    {
  //    }
  //  else
  //    {
  //    }

  struct MHD_Response *response;

  response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN, 32 * 1024, &dir_reader, con_info, &dir_free_callback); 

  if (con_info->response_type) {
    MHD_add_response_header(response, "Content-Type", con_info->response_type);
  }

  int code = MHD_HTTP_OK;

  if (con_info->response_code) {
    code = con_info->response_code;
  }

  ret = MHD_queue_response (connection, code, response);
  MHD_destroy_response (response);

  free_var(result);

  return ret;
}

void
start_httpd_server()
{
    mhd_daemon = MHD_start_daemon (MHD_USE_DEBUG, 8888, NULL, NULL,
				   &ahc_echo, NULL,
				   MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
				   MHD_OPTION_END);
}

void
stop_httpd_server()
{
    MHD_stop_daemon (mhd_daemon);
    mhd_daemon = NULL;
}

/**** built in functions ****/

static package
bf_start_httpd_server(Var arglist, Byte next, void *vdata, Objid progr)
{
    free_var(arglist);
    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    start_httpd_server();
    oklog("Started HTTPD server!\n");
    return no_var_pack();
}

static package
bf_stop_httpd_server(Var arglist, Byte next, void *vdata, Objid progr)
{
    free_var(arglist);
    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    oklog("Stopped HTTPD server!\n");
    stop_httpd_server();
    return no_var_pack();
}

static package
bf_request(Var arglist, Byte next, void *vdata, Objid progr)
{
  Connid id = arglist.v.list[1].v.num;
  char *opt = arglist.v.list[2].v.str;

  if (0 != strcmp(opt, "method") && 0 != strcmp(opt, "uri") && 0 != strcmp(opt, "type") && 0 != strcmp(opt, "body")) {
    free_var(arglist);
    return make_error_pack(E_INVARG);
  }

  if (!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  struct connection_info_struct *con_info = find_connection_info_struct(id);

  if (con_info && 0 == strcmp(opt, "method")) {
    char *method = con_info->request_method ? con_info->request_method : "";
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(method);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "uri")) {
    char *uri = con_info->request_uri ? con_info->request_uri : "";
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(uri);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "body") && con_info->request_body) {
    Var r;
    int len = strlen(con_info->request_body);
    r.type = TYPE_STR;
    r.v.str = str_dup(raw_bytes_to_binary(con_info->request_body, len));
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "body")) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup("");
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "type") && con_info->request_type) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(con_info->request_type);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "type")) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup("");
    free_var(arglist);
    return make_var_pack(r);
  }
  else {
    free_var(arglist);
    return make_error_pack(E_INVARG);
  }
}

static package
bf_response(Var arglist, Byte next, void *vdata, Objid progr)
{
  Connid id = arglist.v.list[1].v.num;
  char *opt = arglist.v.list[2].v.str;

  if (0 != strcmp(opt, "code") && 0 != strcmp(opt, "type") && 0 != strcmp(opt, "body")) {
    free_var(arglist);
    return make_error_pack(E_INVARG);
  }

  if (!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  struct connection_info_struct *con_info = find_connection_info_struct(id);

  if (con_info && 0 == strcmp(opt, "code") && arglist.v.list[3].type == TYPE_INT) {
    con_info->response_code = arglist.v.list[3].v.num;
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "body") && con_info->response_body == NULL) {
    con_info->response_body = strdup(value_to_literal(arglist.v.list[3]));
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "type") && arglist.v.list[3].type == TYPE_STR && con_info->response_type == NULL) {
    con_info->response_type = strdup(arglist.v.list[3].v.str);
    free_var(arglist);
    return no_var_pack();
  }
  else {
    free_var(arglist);
    return make_error_pack(E_INVARG);
  }
}

void
register_httpd(void)
{
    register_function("start_httpd_server", 0, 0, bf_start_httpd_server);
    register_function("stop_httpd_server", 0, 0, bf_stop_httpd_server);
    register_function("request", 2, 2, bf_request, TYPE_INT, TYPE_STR);
    register_function("response", 3, 3, bf_response, TYPE_INT, TYPE_STR, TYPE_ANY);
}
