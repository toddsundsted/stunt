#include "my-types.h"
#include "my-string.h"
#include "my-stdlib.h"

#include "platform.h"
#include <microhttpd.h>

#include "config.h"
#include "functions.h"
#include "list.h"
#include "storage.h"
#include "structures.h"
#include "execute.h"
#include "tasks.h"
#include "log.h"
#include "utils.h"

#include "base64.h"

#include "httpd.h"

typedef unsigned32 Conid;

struct connection_info_struct
{
  Conid id;
  Objid receiver;
  Objid player;
  char *request_method;
  char *request_uri;
  char *request_type;
  char *request_body;
  size_t request_body_length;
  int response_code;
  char *response_location;
  char *response_type;
  char *response_body;
  size_t response_body_length;
  struct connection_info_struct *prev;
  struct connection_info_struct *next;
};

static struct connection_info_struct con_info_head;

struct MHD_Daemon *mhd_daemon;

static void
remove_connection_info_struct(struct connection_info_struct *con_info)
{
  if (con_info) {
    con_info->prev->next = con_info->next;
    con_info->next->prev = con_info->prev;
    if (con_info->request_method) free(con_info->request_method);
    if (con_info->request_uri) free(con_info->request_uri);
    if (con_info->request_type) free(con_info->request_type);
    if (con_info->request_body) free(con_info->request_body);
    if (con_info->response_location) free(con_info->response_location);
    if (con_info->response_type) free(con_info->response_type);
    if (con_info->response_body) free(con_info->response_body);
    free(con_info);
  }
}

static void
add_connection_info_struct(struct connection_info_struct *con_info)
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

static struct connection_info_struct *
new_connection_info_struct()
{
  static Conid max_id = 0;
  struct connection_info_struct *con_info = malloc(sizeof(struct connection_info_struct));
  if (NULL == con_info) return NULL;
  con_info->id = ++max_id;
  con_info->receiver = NOTHING;
  con_info->player = NOTHING;
  con_info->request_method = NULL;
  con_info->request_uri = NULL;
  con_info->request_type = NULL;
  con_info->request_body = NULL;
  con_info->request_body_length = 0;
  con_info->response_code = 0;
  con_info->response_location = NULL;
  con_info->response_type = NULL;
  con_info->response_body = NULL;
  con_info->response_body_length = 0;
  con_info->prev = NULL;
  con_info->next = NULL;
  add_connection_info_struct(con_info);
  return con_info;
}

static struct connection_info_struct *
find_connection_info_struct(Conid id)
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

static int
request_authentication(struct MHD_Connection *connection)
{
  const char *realm = "Basic realm = \"LambdaMoo Server\"";
  struct MHD_Response *response;
  int ret;

  response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
  if (!response)
    return MHD_NO;

  ret = MHD_add_response_header(response, "WWW-Authenticate", realm);
  if (!ret) {
    MHD_destroy_response(response);
    return MHD_NO;
  }

  ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);

  MHD_destroy_response(response);

  return ret;
}

static Objid
authenticated_as(struct MHD_Connection *connection, struct connection_info_struct *con_info)
{
  if (con_info->player != NOTHING)
    return con_info->player;

  size_t dummy;
  const char *strbase = "Basic ";

  const char *headervalue =
    MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
  if (NULL == headervalue)
    return NOTHING;
  if (0 != strncmp(headervalue, strbase, strlen(strbase)))
    return NOTHING;

  const unsigned char *decoded =
    base64_decode(headervalue + strlen(strbase), strlen(headervalue) - strlen(strbase), &dummy);
  if (NULL == decoded)
    return NOTHING;

  char *separator = strchr(decoded, ':');
  if (NULL == separator) {
    free((char *)decoded);
    return NOTHING;
  }
  *separator = 0;
  const unsigned char *username = decoded;
  const unsigned char *password = separator + 1;

  Var args;
  Var result;
  enum outcome outcome;
  Objid player = NOTHING;

  args = new_list(2);
  args.v.list[1].type = TYPE_STR;
  args.v.list[1].v.str = str_dup(username);
  args.v.list[2].type = TYPE_STR;
  args.v.list[2].v.str = str_dup(password);
  outcome = run_server_task(NOTHING, SYSTEM_OBJECT, "authenticate", args, "", &result);
  if (outcome == OUTCOME_DONE && result.type == TYPE_OBJ) {
    player = con_info->player = result.v.obj;
  }

  free_var(result);
  free((char *)decoded);

  return player;
}

static Objid
route_to(struct connection_info_struct *con_info)
{
  if (con_info->receiver != NOTHING)
    return con_info->receiver;

  Var args;
  Var result;
  enum outcome outcome;
  Objid receiver = SYSTEM_OBJECT;

  args = new_list(2);
  args.v.list[1].type = TYPE_STR;
  args.v.list[1].v.str = str_dup(con_info->request_method);
  args.v.list[2].type = TYPE_STR;
  args.v.list[2].v.str = str_dup(con_info->request_uri);
  outcome = run_server_task(NOTHING, SYSTEM_OBJECT, "route", args, "", &result);
  if (outcome == OUTCOME_DONE && result.type == TYPE_OBJ) {
    receiver = con_info->receiver = result.v.obj;
  }

  free_var(result);

  return receiver;
}

static void
response_body_free_callback(void *cls)
{
}

static int
response_body_reader(void *cls, uint64_t pos, char *buf, int max)
{
  struct connection_info_struct *con_info = (struct connection_info_struct *)cls;

  if (con_info->response_body) {
    size_t len = con_info->response_body_length - pos < max ? con_info->response_body_length - pos : max;
    memcpy(buf, con_info->response_body + pos, len);
    return len ? len : -1;
  }
  else {
    return 0;
  }
}

static void *
request_started(void *cls, const char *uri)
{
  struct connection_info_struct *con_info = new_connection_info_struct();

  con_info->request_uri = strdup(raw_bytes_to_binary(uri, strlen(uri)));

  return con_info;
}

static void
request_completed(void *cls, struct MHD_Connection *connection,
		  void **ptr,
		  enum MHD_RequestTerminationCode term_code)
{
  remove_connection_info_struct(*ptr);
}

static int
handle_http_request(void *cls, struct MHD_Connection *connection,
		    const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size,
		    void **ptr)
{
  if (NULL == *ptr) {
    return MHD_NO;
  }

  struct connection_info_struct *con_info = *ptr;

  if (NULL == con_info->request_method) {
    oklog("HTTPD: [%d] %s %s\n", con_info->id, method, url);
    con_info->request_method = strdup(raw_bytes_to_binary(method, strlen(method)));
    return MHD_YES;
  }

  if (authenticated_as(connection, con_info) == NOTHING)
    return request_authentication(connection);

  route_to(con_info);

  if (0 != *upload_data_size) {
    if (NULL != con_info->request_body) {
      char *old = con_info->request_body;
      size_t len = con_info->request_body_length;
      con_info->request_body = malloc(con_info->request_body_length + *upload_data_size);
      con_info->request_body_length = con_info->request_body_length + *upload_data_size;
      memcpy(con_info->request_body, old, len);
      memcpy(con_info->request_body + len, upload_data, *upload_data_size);
      free (old);
    }
    else {
      con_info->request_body = malloc(*upload_data_size);
      con_info->request_body_length = *upload_data_size;
      memcpy(con_info->request_body, upload_data, *upload_data_size);
    }
    *upload_data_size = 0;
    return MHD_YES;
  }

  const char *content_type =
    MHD_lookup_connection_value (connection, MHD_HEADER_KIND, "Content-Type");

  content_type = content_type ? raw_bytes_to_binary(content_type, strlen(content_type)) : "";

  con_info->request_type = strdup(content_type);

  Var args;

  args = new_list(1);
  args.v.list[1].type = TYPE_INT;
  args.v.list[1].v.num = con_info->id;
  run_server_task(con_info->player, con_info->receiver, "do_http", args, "", NULL);

  struct MHD_Response *response =
    MHD_create_response_from_callback (MHD_SIZE_UNKNOWN, 32 * 1024, &response_body_reader, con_info, &response_body_free_callback); 

  if (con_info->response_location) {
    MHD_add_response_header(response, "Location", con_info->response_location);
  }

  if (con_info->response_type) {
    MHD_add_response_header(response, "Content-Type", con_info->response_type);
  }

  int code = MHD_HTTP_OK;

  if (con_info->response_code) {
    code = con_info->response_code;
  }

  int ret = MHD_queue_response (connection, code, response);
  MHD_destroy_response (response);

  return ret;
}

struct MHD_Daemon *
start_httpd_server(int port)
{
  struct MHD_OptionItem ops[] = {
    { MHD_OPTION_URI_LOG_CALLBACK, (intptr_t)&request_started, NULL },
    { MHD_OPTION_NOTIFY_COMPLETED, (intptr_t)&request_completed, NULL },
    { MHD_OPTION_CONNECTION_TIMEOUT, 30 },
    { MHD_OPTION_END, 0, NULL }
  };

  mhd_daemon = MHD_start_daemon(MHD_NO_FLAG, port,
				NULL, NULL,
				&handle_http_request, NULL,
				MHD_OPTION_ARRAY, ops,
				MHD_OPTION_END);

  return mhd_daemon;
}

void
stop_httpd_server()
{
  if (mhd_daemon) {
    MHD_stop_daemon(mhd_daemon);
    mhd_daemon = NULL;
  }
}

/**** built in functions ****/

static package
bf_start_httpd_server(Var arglist, Byte next, void *vdata, Objid progr)
{
  int port = arglist.v.list[1].v.num;
  free_var(arglist);
  if (!is_wizard(progr)) {
    return make_error_pack(E_PERM);
  }
  if (start_httpd_server(port) == NULL) {
    return make_error_pack(E_INVARG);
  }
  oklog("HTTPD: now running on port %d\n", port);
  return no_var_pack();
}

static package
bf_stop_httpd_server(Var arglist, Byte next, void *vdata, Objid progr)
{
  free_var(arglist);
  if (!is_wizard(progr)) {
    return make_error_pack(E_PERM);
  }
  stop_httpd_server();
  oklog("HTTPD: no longer running\n");
  return no_var_pack();
}

static package
bf_request(Var arglist, Byte next, void *vdata, Objid progr)
{
  Conid id = arglist.v.list[1].v.num;
  const char *opt = arglist.v.list[2].v.str;

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
  else if (con_info && 0 == strcmp(opt, "type")) {
    char *type = con_info->request_type ? con_info->request_type : "";
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(type);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "body") && con_info->request_body) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(raw_bytes_to_binary(con_info->request_body, con_info->request_body_length));
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
  else {
    free_var(arglist);
    return make_error_pack(E_INVARG);
  }
}

static package
bf_response(Var arglist, Byte next, void *vdata, Objid progr)
{
  Conid id = arglist.v.list[1].v.num;
  const char *opt = arglist.v.list[2].v.str;

  if (0 != strcmp(opt, "code") && 0 != strcmp(opt, "location") && 0 != strcmp(opt, "type") && 0 != strcmp(opt, "body")) {
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
  else if (con_info && 0 == strcmp(opt, "location") && arglist.v.list[3].type == TYPE_STR && con_info->response_type == NULL) {
    con_info->response_location = strdup(arglist.v.list[3].v.str);
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "type") && arglist.v.list[3].type == TYPE_STR && con_info->response_type == NULL) {
    con_info->response_type = strdup(arglist.v.list[3].v.str);
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "body") && arglist.v.list[3].type == TYPE_STR && con_info->response_body == NULL) {
    int len;
    const char *buff = binary_to_raw_bytes(arglist.v.list[3].v.str, &len);
    con_info->response_body = malloc(len);
    con_info->response_body_length = len;
    memcpy(con_info->response_body, buff, len);
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
  register_function("start_httpd_server", 1, 1, bf_start_httpd_server, TYPE_INT);
  register_function("stop_httpd_server", 0, 0, bf_stop_httpd_server);
  register_function("request", 2, 2, bf_request, TYPE_INT, TYPE_STR);
  register_function("response", 3, 3, bf_response, TYPE_INT, TYPE_STR, TYPE_ANY);
}
