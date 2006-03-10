 
enum option_id {
  OPT_NAME = 1,
  OPT_GROUP,
  OPT_HANDLE
};
typedef enum option_id          option_id;

struct option_list {
  const char *    option;
  option_id       id;
  const char *    def;
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

