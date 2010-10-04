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

#include "my-string.h"

#include "platform.h"
#include <microhttpd.h>

#include "uthash.h"

#include "config.h"
#include "execute.h"
#include "functions.h"
#include "hash.h"
#include "httpd.h"
#include "list.h"
#include "log.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "tasks.h"
#include "utils.h"

typedef unsigned32 Conid;

struct form_body_struct {
  char *key;
  char *filename;
  char *content_type;
  Stream *data;
  UT_hash_handle hh;
};

struct connection_info_struct
{
  Conid id;
  struct MHD_Connection *connection;
  struct MHD_PostProcessor *post_processor;
  char *request_method;
  char *request_uri;
  char *request_type;
  char *request_body;
  size_t request_body_length;
  int response_code;
  Var response_headers;
  char *response_body;
  size_t response_body_length;
  struct form_body_struct *form_body;
  struct connection_info_struct *prev;
  struct connection_info_struct *next;
};

static struct connection_info_struct con_info_head;

static Var handler_list;

struct MHD_Daemon *mhd_daemon;

static void
remove_connection_info_struct(struct connection_info_struct *con_info)
{
  if (con_info) {
    con_info->prev->next = con_info->next;
    con_info->next->prev = con_info->prev;
    if (con_info->request_method) free_str(con_info->request_method);
    if (con_info->request_uri) free_str(con_info->request_uri);
    if (con_info->request_type) free_str(con_info->request_type);
    if (con_info->request_body) free_str(con_info->request_body);
    free_var(con_info->response_headers);
    if (con_info->response_body) free_str(con_info->response_body);
    struct form_body_struct *fb, *tmp;
    HASH_FOREACH_SAFE(hh, con_info->form_body, fb, tmp) {
      HASH_DELETE(hh, con_info->form_body, fb);
      if (fb->key) free_str(fb->key);
      if (fb->filename) free_str(fb->filename);
      if (fb->content_type) free_str(fb->content_type);
      if (fb->data) free_stream(fb->data);
      myfree(fb, M_APPLICATION_DATA);
    }
    myfree(con_info, M_APPLICATION_DATA);
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
  struct connection_info_struct *con_info = mymalloc(sizeof(struct connection_info_struct), M_APPLICATION_DATA);
  if (NULL == con_info) return NULL;
  con_info->id = ++max_id;
  con_info->connection = NULL;
  con_info->post_processor = NULL;
  con_info->request_method = NULL;
  con_info->request_uri = NULL;
  con_info->request_type = NULL;
  con_info->request_body = NULL;
  con_info->request_body_length = 0;
  con_info->response_code = 0;
  con_info->response_headers = new_hash();
  con_info->response_body = NULL;
  con_info->response_body_length = 0;
  con_info->form_body = NULL;
  con_info->prev = NULL;
  con_info->next = NULL;
  add_connection_info_struct(con_info);
  return con_info;
}

static struct connection_info_struct *
find_connection_info_struct(Conid id)
{
  if (con_info_head.prev == NULL || con_info_head.next == NULL)
    return NULL;
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
iterate_form_body(void *cls, enum MHD_ValueKind kind, 
		  const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, 
		  uint64_t off, size_t size)
{
  struct connection_info_struct *con_info = (struct connection_info_struct *)cls;
  struct form_body_struct *fb;

  HASH_FIND_STR(con_info->form_body, key, fb);

  if (!fb) {
    fb = mymalloc(sizeof(struct form_body_struct), M_APPLICATION_DATA);
    fb->key = str_dup(raw_bytes_to_binary(key, strlen(key)));
    fb->filename = filename ? str_dup(raw_bytes_to_binary(filename, strlen(filename))) : str_dup("");
    fb->content_type = content_type ? str_dup(raw_bytes_to_binary(content_type, strlen(content_type))) : str_dup("");
    fb->data = new_stream(size);
    HASH_ADD_KEYPTR(hh, con_info->form_body, fb->key, strlen(fb->key), fb);
  }
  stream_add_string(fb->data, raw_bytes_to_binary(data, size));

  return MHD_YES;
}

static int
iterator_callback(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
  Var *h = (Var *)cls;
  Var k, v;
  k.type = TYPE_STR;
  k.v.str = str_dup(raw_bytes_to_binary(key, strlen(key)));
  v.type = TYPE_STR;
  v.v.str = str_dup(raw_bytes_to_binary(value, strlen(value)));
  hashinsert(*h, k, v);
  return MHD_YES;
}

static void
hash_headers_callback(Var key, Var value, void *data, int32 first)
{
  struct MHD_Response *response = (struct MHD_Response *)data;

  MHD_add_response_header(response, key.v.str, value.v.str);
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

  con_info->request_uri = str_dup(raw_bytes_to_binary(uri, strlen(uri)));

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

  if (NULL == con_info->connection) {
    con_info->connection = connection;
    con_info->post_processor = MHD_create_post_processor(connection, 1024, iterate_form_body, con_info);
    con_info->request_method = str_dup(raw_bytes_to_binary(method, strlen(method)));
    oklog("HTTPD: [%d] %s %s\n", con_info->id, con_info->request_method, con_info->request_uri);
    return MHD_YES;
  }

  if (0 != *upload_data_size) {
    if (NULL != con_info->request_body) {
      char *body = con_info->request_body;
      size_t body_length = con_info->request_body_length;
      con_info->request_body = mymalloc(con_info->request_body_length + *upload_data_size, M_STRING);
      con_info->request_body_length = con_info->request_body_length + *upload_data_size;
      memcpy(con_info->request_body, body, body_length);
      memcpy(con_info->request_body + body_length, upload_data, *upload_data_size);
      free_str(body);
    }
    else {
      con_info->request_body = mymalloc(*upload_data_size, M_STRING);
      con_info->request_body_length = *upload_data_size;
      memcpy(con_info->request_body, upload_data, *upload_data_size);
    }
    if (con_info->post_processor) MHD_post_process(con_info->post_processor, upload_data, *upload_data_size);
    *upload_data_size = 0;
    return MHD_YES;
  }
  else {
    if (con_info->post_processor) MHD_destroy_post_processor(con_info->post_processor);
    con_info->post_processor = NULL;
  }

  const char *content_type = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, "Content-Type");

  content_type = content_type ? raw_bytes_to_binary(content_type, strlen(content_type)) : "";

  con_info->request_type = str_dup(content_type);

  Var call_list = var_ref(handler_list);

  while (/* list must have at least one sub-list */
	 call_list.type == TYPE_LIST &&
	 call_list.v.list[0].type == TYPE_INT && call_list.v.list[0].v.num > 0 &&
 	 /* sub-list must have at least three members: TYPE_OBJ (player), TYPE_OBJ (receiver), TYPE_STR (verb) */
	 call_list.v.list[1].type == TYPE_LIST &&
	 call_list.v.list[1].v.list[0].type == TYPE_INT && call_list.v.list[1].v.list[0].v.num > 2 &&
	 call_list.v.list[1].v.list[1].type == TYPE_OBJ &&
	 call_list.v.list[1].v.list[2].type == TYPE_OBJ &&
	 call_list.v.list[1].v.list[3].type == TYPE_STR) {
    Objid player = call_list.v.list[1].v.list[1].v.obj;
    Objid receiver = call_list.v.list[1].v.list[2].v.obj;
    const char *verb = str_ref(call_list.v.list[1].v.list[3].v.str);

    Var args;

    args = new_list(1);
    args.v.list[1].type = TYPE_INT;
    args.v.list[1].v.num = con_info->id;

    Var temp;

    temp = var_ref(call_list.v.list[1]);
    call_list = listdelete(call_list, 1);
    args = listappend(args, var_ref(call_list));
    args = listappend(args, temp);

    Var result;

    result = zero; /* standard procedure before run_server_task in case outcome is not OUTCOME_DONE */

    enum outcome outcome = run_server_task(player, receiver, verb, args, "", &result);

    if (OUTCOME_DONE == outcome && is_true(result)) {
      free_var(call_list);
      call_list = var_ref(result);
    }

    free_var(result);
    free_str(verb);
  }

  free_var(call_list);

  int code = MHD_HTTP_OK;

  struct MHD_Response *response;

  if (con_info->response_code) {
    code = con_info->response_code;
  }

  if (con_info->response_body) {
    response =
      MHD_create_response_from_data(con_info->response_body_length, con_info->response_body, MHD_NO, MHD_NO);
  }
  else {
    response =
      MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 32 * 1024, &response_body_reader, con_info, &response_body_free_callback); 
  }

  hashforeach(con_info->response_headers, hash_headers_callback, response);

  int ret = MHD_queue_response (connection, code, response);
  MHD_destroy_response (response);

  return ret;
}

struct MHD_Daemon *
start_httpd_server(int port, Var list)
{
  if (mhd_daemon) {
    return NULL;
  }

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
  handler_list = var_ref(list);
  return mhd_daemon;
}

void
stop_httpd_server()
{
  if (mhd_daemon) {
    MHD_stop_daemon(mhd_daemon);
    free_var(handler_list);
    mhd_daemon = NULL;
  }
}

/**** built in functions ****/

static package
bf_start_httpd_server(Var arglist, Byte next, void *vdata, Objid progr)
{
  int port = arglist.v.list[1].v.num;
  Var list = arglist.v.list[2];
  if (!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }
  if (start_httpd_server(port, list) == NULL) {
    free_var(arglist);
    return make_error_pack(E_INVARG);
  }
  oklog("HTTPD: started on port %d\n", port);
  free_var(arglist);
  return no_var_pack();
}

static package
bf_stop_httpd_server(Var arglist, Byte next, void *vdata, Objid progr)
{
  if (!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }
  stop_httpd_server();
  oklog("HTTPD: stopped\n");
  free_var(arglist);
  return no_var_pack();
}

static package
bf_request(Var arglist, Byte next, void *vdata, Objid progr)
{
  if (!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  Conid id = arglist.v.list[1].v.num;
  const char *opt = arglist.v.list[2].v.str;

  struct connection_info_struct *con_info = find_connection_info_struct(id);

  if (con_info && 0 == strcmp(opt, "headers")) {
    Var r = new_hash();
    MHD_get_connection_values(con_info->connection, MHD_HEADER_KIND, iterator_callback, (void *)&r);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "cookies")) {
    Var r = new_hash();
    MHD_get_connection_values(con_info->connection, MHD_COOKIE_KIND, iterator_callback, (void *)&r);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "path")) {
    char *uri = con_info->request_uri ? con_info->request_uri : "";
    int n = strlen(uri);
    char buffer[n + 1];
    int i;
    for (i = 0; uri[i] && '?' != uri[i]; i++)
      ;
    strncpy(buffer, uri, i);
    buffer[i] = 0;
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(buffer);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "query")) {
    char *uri = con_info->request_uri ? con_info->request_uri : "";
    int n = strlen(uri);
    char buffer[n + 1];
    int i;
    for (i = 0; uri[i] && '?' != uri[i]; i++)
      ;
    strcpy(buffer, uri + i + 1);
    buffer[n - i] = 0;
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(buffer);
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "method")) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = con_info->request_method ? str_ref(con_info->request_method) : str_dup("");
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "uri")) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = con_info->request_uri ? str_ref(con_info->request_uri) : str_dup("");
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "type")) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = con_info->request_type ? str_ref(con_info->request_type) : str_dup("");
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "body")) {
    Var r;
    r.type = TYPE_STR;
    r.v.str = con_info->request_body ? str_dup(raw_bytes_to_binary(con_info->request_body, con_info->request_body_length)) : str_dup("");
    free_var(arglist);
    return make_var_pack(r);
  }
  else if (con_info && 0 == strcmp(opt, "form")) {
    Var r = new_hash();
    struct form_body_struct *fb;
    HASH_FOREACH(hh, con_info->form_body, fb) {
      Var key, value;
      key.type = TYPE_STR;
      key.v.str = str_ref(fb->key);
      if (fb->filename && *fb->filename) {
	value = new_list(3);
	value.v.list[1].type = TYPE_STR;
	value.v.list[1].v.str = str_ref(fb->filename);
	value.v.list[2].type = TYPE_STR;
	value.v.list[2].v.str = str_ref(fb->content_type);
	value.v.list[3].type = TYPE_STR;
	value.v.list[3].v.str = str_dup(stream_contents(fb->data));
      }
      else {
	value.type = TYPE_STR;
	value.v.str = str_dup(stream_contents(fb->data));
      }
      hashinsert(r, key, value);
      free_var(key);
      free_var(value);
    }
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
  static Var location;
  static Var type;
  static int initialized = 0;
  if (!initialized) {
    initialized = 1;
    location.type = TYPE_STR;
    location.v.str = str_dup("Location");
    type.type = TYPE_STR;
    type.v.str = str_dup("Content-Type");
  }

  if (!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  Conid id = arglist.v.list[1].v.num;
  const char *opt = arglist.v.list[2].v.str;

  struct connection_info_struct *con_info = find_connection_info_struct(id);

  if (con_info && 0 == strcmp(opt, "code") && 3 == arglist.v.list[0].v.num && TYPE_INT == arglist.v.list[3].type) {
    con_info->response_code = arglist.v.list[3].v.num;
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "header") && 4 == arglist.v.list[0].v.num && TYPE_STR == arglist.v.list[3].type && TYPE_STR == arglist.v.list[4].type) {
    hashinsert(con_info->response_headers, var_ref(arglist.v.list[3]), var_ref(arglist.v.list[4]));
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "location") && 3 == arglist.v.list[0].v.num && TYPE_STR == arglist.v.list[3].type) {
    hashinsert(con_info->response_headers, var_ref(location), var_ref(arglist.v.list[3]));
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "type") && 3 == arglist.v.list[0].v.num && TYPE_STR == arglist.v.list[3].type) {
    hashinsert(con_info->response_headers, var_ref(type), var_ref(arglist.v.list[3]));
    free_var(arglist);
    return no_var_pack();
  }
  else if (con_info && 0 == strcmp(opt, "body") && 3 == arglist.v.list[0].v.num && TYPE_STR == arglist.v.list[3].type) {
    if (con_info->response_body) free_str(con_info->response_body);
    int length;
    const char *buffer = binary_to_raw_bytes(arglist.v.list[3].v.str, &length);
    con_info->response_body = mymalloc(length, M_STRING);
    con_info->response_body_length = length;
    memcpy(con_info->response_body, buffer, length);
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
  register_function("start_httpd_server", 2, 2, bf_start_httpd_server, TYPE_INT, TYPE_LIST);
  register_function("stop_httpd_server", 0, 0, bf_stop_httpd_server);
  register_function("request", 2, 2, bf_request, TYPE_INT, TYPE_STR);
  register_function("response", 3, 4, bf_response, TYPE_INT, TYPE_STR, TYPE_ANY, TYPE_ANY);
}
