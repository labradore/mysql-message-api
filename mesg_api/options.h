#if defined UNIT_TEST
#include "harness.h"
#else

#include <my_global.h> 
#include <my_sys.h>

#endif

#include <string.h>

enum option_id {
  OPT_NAME = 1,
  OPT_GROUP,
  OPT_HANDLE
};
typedef enum option_id          option_id;

struct option_list {
  const char *    name;
  option_id       id;
  int             max_value_len;
  char *          value;
  int             value_len;
};
typedef struct option_list      option_list;  

enum opt_parser_return {
  PARS_OK = 0,
  PARS_ILLEGAL_OPTION,
  PARS_SYNTAX_ERROR,
  
};
typedef enum opt_parser_return  opt_parser_return;

