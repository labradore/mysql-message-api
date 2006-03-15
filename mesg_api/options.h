#if defined UNIT_TEST
#include "harness.h"
#else

#include <my_global.h> 
#include <my_sys.h>

#endif

#include <string.h>

struct option_list {
  const char *    name;
  int             max_value_len;
  char *          value;
  unsigned int    value_len;
};
typedef struct option_list      option_list;  

enum opt_parser_return {
  PARS_OK = 0,
  PARS_ILLEGAL_OPTION,
  PARS_SYNTAX_ERROR,
  
};
typedef enum opt_parser_return  opt_parser_return;

/* Prototypes */
opt_parser_return parse_options(int, option_list *, 
                                unsigned int, unsigned int *, char *);


