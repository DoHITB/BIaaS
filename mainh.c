/* Include the required headers from httpd */
#include "httpd.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "stdio.h"
#include "stdio.h"
#include "dirent.h"
#include "sys/types.h"
#include "stdlib.h"
#include "unistd.h"

#include "apr_strings.h"
#include "apr_network_io.h"
#include "apr_dbd.h"
#include <apr_file_info.h>
#include <apr_file_io.h>
#include <apr_tables.h>

/* Define prototypes of our functions in this module */
static void register_hooks(apr_pool_t *pool);
static int calc_handler(request_rec *r);

/* Internal functions */
static int validateFileName(const char* fileName, request_rec *r);
static int isClear(const char* fileName);
static int store(const char* operation, const char* op1, const char* op2, const char* fileName);
static int retreive(const char* fileName, request_rec *r);

/* Define our module as an entity and assign a function for registering hooks  */
module AP_MODULE_DECLARE_DATA calc_module = {STANDARD20_MODULE_STUFF, NULL, NULL, NULL, NULL, NULL, register_hooks};

/* register_hooks: Adds a hook to the httpd process */
static void register_hooks(apr_pool_t *pool) {
  /* Hook the request handler */
  ap_hook_handler(calc_handler, NULL, NULL, APR_HOOK_LAST);
}

/*
 * The handler function for our module.
 * This is where all the fun happens!
 */
static int calc_handler(request_rec *r) {
  apr_table_t* GET;
  const char* kind;
  const char* fileName;
  const char* operation;
  const char* op1;
  const char* op2;
  int fnRet = 0;
  int retres = 0;

  /*
   * First off, we need to check if this is a call for the "calc" handler.
   * If it is, we accept it and do our things, it not, we simply return DECLINED,
   * and Apache will try somewhere else.
   */
  if (!r->handler || strcmp(r->handler, "calc-handler")) return (DECLINED);

  //set the correct data type
  ap_set_content_type(r, "application/json; charset=utf-8");

  //Debug traces (OFF)
  //ap_rputs("Inside Handler <br />", r);

  //Parse GET values
  ap_args_to_table(r, &GET);

  //get "kind" parameter. If not set, will be "void"
  kind = apr_table_get(GET, "kind");

  if(!kind || strlen(kind) == 0 || strcmp(kind, " ") == 0)
    kind = "void";

  //JSON starts
  ap_rprintf(r, "{\"request\" : \"%s\", ", kind);

  //check which operation the user requested
  if(strcmp(kind, "makeOperation") == 0){
    /*
     * Make Operation. Expected data:
     *   fileName --> Name of the session file. Should not exceed 128 characters long.
     *                Must be new.
     *
     *   operation --> kind of operation to make. Valid values
     *                 +, -, /, *, ^, s
     *
     *   op1 --> first operand
     *
     *   op2 --> second operand. If operation is "^" or "s" should be a integer value.
     */
    fileName = apr_table_get(GET, "fileName");

    //no "fileName" was given -> error and end processing
    if(!fileName || strlen(fileName) == 0 || strcmp(fileName, " ") == 0){
      ap_rputs("\"error\" : \"err_fileName_not_set\"}", r);

      return OK;
    }

    //we must validate that file exists
    fnRet = validateFileName(fileName, r);

    //fnRet != 0 means file exists -> error and end processing
    if(fnRet != 0){
      ap_rputs("\"error\" : \"err_fileName_not_set\"}", r);

      return OK;
    }

    //get "operation", "op1", and "op2"
    operation = apr_table_get(GET, "operation");
    op1 = apr_table_get(GET, "op1");
    op2 = apr_table_get(GET, "op2");

    //no "operation" was given -> error and stop processing
    if(!operation || strlen(operation) == 0 || strcmp(operation, " ") == 0){
      ap_rputs("\"error\" : \"err_operation_not_set\"}", r);

      return OK;
    }

    //no "op1" was given -> error and stop processing
    if(!op1 || strlen(op1) == 0 || strcmp(op1, " ") == 0){
      ap_rputs("\"error\" : \"err_op1_not_set\"}", r);

      return OK;
    }

    //no "op2" was given -> error and stop processing
    if(!op2 || strlen(op2) == 0 || strcmp(op2, " ") == 0){
      ap_rputs("\"error\" : \"err_op2_not_set\"}", r);

      return OK;
    }

    //check length of fields
    if(strlen(operation) > 1){
      ap_rputs("\"error\" : \"err_unknown_operation\"}", r);

      return OK;
    }

    if(strlen(op1) > 1024){
      ap_rputs("\"error\" : \"err_op1_not_set\"}", r);

      return OK;
    }

    if(strlen(op2) > 1024){
      ap_rputs("\"error\" : \"err_op2_not_set\"}", r);

      return OK;
    }

    //we only operate if the "operation" is inside valid range
    if(operation[0] == '+' || operation[0] == '-' || operation[0] == '*' || operation[0] == '/' || operation[0] == '^' || operation[0] == 's'){
      store(operation, op1, op2, fileName);
    }else{
      ap_rputs("\"error\" : \"err_unknown_operation\"}", r);

      return OK;
    }
  }else if(strcmp(kind, "getResult") == 0){
    /*
     * Get result. Expected data:
     *   fileName --> Name of the session file. Should not exceed 128 characters long.
     *                Must exists.
     */
    fileName = apr_table_get(GET, "fileName");

    //no "fileName" was given -> error and end processing
    if(!fileName || strlen(fileName) == 0){
      ap_rputs("\"error\" : \"err_fileName_not_set\"}", r);

      return OK;
    }

    //we must validate that file exists
    fnRet = validateFileName(fileName, r);

    //fnRet != -2 means that file does not exist -> error and stop processing
    if(fnRet != -2){
      ap_rputs("\"error\" : \"err_fileName_not_set\"}", r);

      return OK;
    }

    //get the result
    retres = retreive(fileName, r);
  }else{
    //unknown or "void" request -> error and stop processing
    ap_rputs("\"error\" : \"unknown_request\"}", r);

    return OK;
  }

  //it will only reach this point if everything is ok.
  if(retres == 0)
    ap_rputs("\"status\" : \"ok\"}", r);

  return OK;
}

/*
 * validateFileName.
 *
 * Checks data regarding "fileName" parameter.
 */
static int validateFileName(const char* fileName, request_rec *r){
  int rc;
  int exists;
  char* vars = malloc(sizeof(char) * 129);
  apr_finfo_t finfo;

  /*
   * Validations
   *   - File name shall not exceed 126 characters long.
   *   - File name shall not exist not in /f folder, nor in /r folder.
   */
  if(strlen(fileName) > 126){
    free(vars);
    return -1;
  }

  if(isClear(fileName) != 0){
    free(vars);
    return -2;
  }

  //check if file exists on /f folder or in /r folder
  snprintf(vars, strlen(fileName) + 14, "/var/www/c/f/%s", fileName);
  apr_pstrdup(r->pool, vars);

  rc = apr_stat(&finfo, vars, APR_FINFO_MIN, r->pool);

  if (rc == APR_SUCCESS) {
    exists = ((finfo.filetype != APR_NOFILE) &&  !(finfo.filetype & APR_DIR));

    if (!exists){
      //file not found on /f. Let's search on /r
      snprintf(vars, strlen(fileName) + 16, "/var/www/c/r/r_%s", fileName);
      apr_pstrdup(r->pool, vars);

      rc = apr_stat(&finfo, vars, APR_FINFO_MIN, r->pool);

      if (rc == APR_SUCCESS)
        exists = ((finfo.filetype != APR_NOFILE) &&  !(finfo.filetype & APR_DIR));
      else
        exists = 0;
    }
  }else{
    //file not found on /f. Let's search on /r
    snprintf(vars, strlen(fileName) + 16, "/var/www/c/r/r_%s", fileName);
    apr_pstrdup(r->pool, vars);

    rc = apr_stat(&finfo, vars, APR_FINFO_MIN, r->pool);

    if (rc == APR_SUCCESS)
      exists = ((finfo.filetype != APR_NOFILE) &&  !(finfo.filetype & APR_DIR));
    else
      exists = 0;
  }

  if(exists){
    free(vars);
    return -2;
  }

  free(vars);

  return 0;
}

/*
 * isClear.
 *
 * Checks for special characters that may not be on fileName
 */
static int isClear(const char* fileName){
  int ici = 0;
  int icm = strlen(fileName);

  for(ici = 0;ici < icm;ici++)
    if(fileName[ici] == '.' ||
       fileName[ici] == '/')
      return -1;

  return 0;
}

/*
 * store.
 *
 * Formats a file (%s %s %s format) with name "fileName" on /f folder.
 * It will be processed by plist process.
 */
static int store(const char* operation, const char* op1, const char* op2, const char* fileName){
  char* rt = malloc(sizeof(char) * 129);

  //2 BigInteger strings (4096 each) + operator + EOL
  char* buff = malloc(sizeof(char) * 4096 * 2 + 2);
  FILE* w;

  snprintf(rt, strlen(fileName) + 14, "/var/www/c/f/%s", fileName);

  //4 = operator (1) + 2 blanks + EOL
  snprintf(buff, strlen(op1) + strlen(op2) + 4, "%s %s %s", operation, op1, op2);

  w = fopen(rt, "w");
  fputs(buff, w);
  fclose(w);

  free(buff);
  free(rt);
}

/*
 * retreive
 *
 * Checks result on /r folder.
 */
static int retreive(const char* fileName, request_rec *r){
  char* rt = malloc(sizeof(char) * 129);
  char* buff = malloc(sizeof(char) * 4097);
  int ret;
  FILE *rf;

  snprintf(rt, strlen(fileName) + 16, "/var/www/c/r/r_%s", fileName);
  rf = fopen(rt, "r");

  //the result is still in progress
  if(rf == NULL){
    ap_rputs("\"error\" : \"operation_in_progress\"}", r);

    free(buff);
    free(rt);

    return -1;
  }

  fscanf(rf, "%s", buff);

  if(buff[0] == 'B'){
    ap_rprintf(r, "\"error\" : \"%s\"} ", buff);
    ret = -1;
  }else{
    ap_rprintf(r, "\"result\" : \"%s\", ", buff);
    ret = 0;
  }

  fclose(rf);
  remove(rt);

  free(buff);
  free(rt);

  return ret;
}
