#include "my-types.h"
#include "my-string.h"

#include "platform.h"
#include <microhttpd.h>

#include "config.h"
#include "functions.h"
#include "list.h"
#include "storage.h"
#include "structures.h"
#include "tasks.h"
#include "utils.h"

#include "httpd.h"

struct MHD_Daemon *mhd_daemon;

struct connection_info_struct
{
  int connectiontype;
  char *echostring;
};

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

#define PAGE ""

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  const char *page = PAGE;
  struct MHD_Response *response;
  int ret;
  Var args;

  struct connection_info_struct *con_info;

  if(NULL == *ptr) 
    {
      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info) return MHD_NO;
      con_info->echostring = NULL;
      *ptr = (void *) con_info; 
      return MHD_YES;
    }

  con_info = *ptr;
      
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

  if (con_info->echostring)
    {
      int len = strlen(con_info->echostring);
      args = new_list(1);
      args.v.list[1].type = TYPE_STR;
      args.v.list[1].v.str = str_dup(raw_bytes_to_binary(con_info->echostring, len));
      run_server_task(-1, SYSTEM_OBJECT, "do_httpd", args, "", 0);
    }
  else
    {
      run_server_task(-1, SYSTEM_OBJECT, "do_httpd", new_list(0), "", 0);
    }

  if (con_info->echostring)
    {
      response = MHD_create_response_from_data (strlen (con_info->echostring), (void *) con_info->echostring, MHD_NO, MHD_NO);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }
  else
    {
      response = MHD_create_response_from_data (strlen (page), (void *) page, MHD_NO, MHD_NO);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }

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
    return no_var_pack();
}

void
register_httpd(void)
{
    register_function("start_httpd_server", 0, 0, bf_start_httpd_server);
    register_function("stop_httpd_server", 0, 0, bf_stop_httpd_server);
}
