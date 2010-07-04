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

struct connection_info_struct
{
  int connectiontype;
  Objid listener;
  char *echostring;
};

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
    con_info->listener = result.v.obj;
    authenticated = 1;
  }

  free_var(result);

  free((char *)decoded);

  return authenticated;
}

static void
request_completed (void *cls,
		   struct MHD_Connection *connection,
		   void **ptr,
		   enum MHD_RequestTerminationCode toe)
{
  struct connection_info_struct *con_info = (struct connection_info_struct *) *ptr;

  if (NULL == con_info) return;
  if (con_info->echostring) free (con_info->echostring);
  free (con_info);
  *ptr = NULL;   
}

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  const char *page = "";
  struct MHD_Response *response;
  int ret;
  Var args;

  struct connection_info_struct *con_info;

  if(NULL == *ptr) 
    {
      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info) return MHD_NO;
      con_info->listener = -1;
      con_info->echostring = NULL;
      *ptr = (void *) con_info; 
      return MHD_YES;
    }

  con_info = *ptr;
      
  if (!is_authenticated (connection, con_info))
    return ask_for_authentication (connection, "lambdaMoo");

  if (*upload_data_size != 0)
    {
      if (con_info->echostring)
	{
	  char *oldstring = con_info->echostring;
	  size_t len = strlen(oldstring) + *upload_data_size + 1;
	  con_info->echostring = malloc(len);
	  con_info->echostring[0] = 0;
	  strcat(con_info->echostring, oldstring);
	  strncat(con_info->echostring, upload_data, *upload_data_size);
	  con_info->echostring[len - 1] = 0;
	  free (oldstring);
	}
      else
	{
	  size_t len = *upload_data_size + 1;
	  con_info->echostring = malloc(len);
	  con_info->echostring[0] = 0;
	  strncat(con_info->echostring, upload_data, *upload_data_size);
	  con_info->echostring[len - 1] = 0;
	}

      *upload_data_size = 0;
      return MHD_YES;
    }
  else
    {
    }

  Var result;
  enum outcome outcome;

  if (con_info->echostring)
    {
      int len = strlen(con_info->echostring);
      args = new_list(1);
      args.v.list[1].type = TYPE_STR;
      args.v.list[1].v.str = str_dup(raw_bytes_to_binary(con_info->echostring, len));
      outcome = run_server_task(con_info->listener, SYSTEM_OBJECT, "do_httpd", args, "", &result);
    }
  else
    {
      outcome = run_server_task(con_info->listener, SYSTEM_OBJECT, "do_httpd", new_list(0), "", &result);
    }

  if (outcome == OUTCOME_DONE)
    {
      char * foo = value_to_literal(result);
      response = MHD_create_response_from_data (strlen (foo), (void *) foo, MHD_NO, MHD_NO);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }
  else
    {
      response = MHD_create_response_from_data (strlen (page), (void *) page, MHD_NO, MHD_NO);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }

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

void
register_httpd(void)
{
    register_function("start_httpd_server", 0, 0, bf_start_httpd_server);
    register_function("stop_httpd_server", 0, 0, bf_stop_httpd_server);
}
