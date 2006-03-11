#include "options.h"

extern CHARSET_INFO my_charset_latin1;

/* binsearch():
   Binary search through a sorted list of keywords.
   This uses the OS strncasecmp -- mysql has a my_strcasecmp() for latin1,
   but apparently not a version with counted lengths 
*/
int binsearch(char *word, int len, option_list *kp, int n) {
  int cond, low, mid, high;

  low = 0;
  high = n - 1;
  while (low <= high) {
    mid = (low + high) / 2;
    if ((cond = strncasecmp(word, kp[mid].name,len)) < 0)
        high = mid - 1;
    else if (cond > 0)
        low = mid + 1;
    else
        return mid;
  }
  return -1;
}

/* 
   parse_options()
   
   This parser handles the syntax "option = value [, option = value ] ...".
   The option parser does not allocate or copy any strings.  If an option is
   found in the string *s, then opts[idx].value will be set to a pointer 
   (within s) to the given value, and opts[idx].value_len will indicate its 
   length, where idx is the index in opts[] of the named option.
   opts[] is searched as a btree, so it MUST be sorted (ascending & 
   case-insensitive) on the option name.
*/
#define skip_whitespace(p) while(*p && my_isspace(&my_charset_latin1,*p)) p++ 

opt_parser_return parse_options( int n_opts, option_list *opts, char *s) {

  char *opt, *val, *r;
  int i, j;
  int optidx;

  opt = s;

  do {
    i = j = 0 ;
    skip_whitespace(opt);

    /* the first word is the option name, extending from "p" for length "i" */
    for( val = opt; *val && my_isvar(&my_charset_latin1,*val) ; val++, i++);	

    /* The option is the string of length "i" beginning at "opt" */
    if(i == 0)          return PARS_SYNTAX_ERROR;
    optidx = binsearch(opt, i, opts, n_opts);
    if(optidx == -1)    return PARS_ILLEGAL_OPTION;
        
    /* Next, there must be an equals sign */
    skip_whitespace(val);
    if(*val == '=') val++;
    else                return PARS_SYNTAX_ERROR;
    skip_whitespace(val);
 
    /* Everything until the next comma or whitespace is the value */ 
    for( r = val ; *r && *r != ',' && (! my_isspace(&my_charset_latin1,*r)) ; r++, j++);

    /* The value is the string of length "j" beginning at "val" */
    opts[optidx].value = val;
    opts[optidx].value_len =
      ( j > opts[optidx].max_value_len ? opts[optidx].max_value_len : j );
    
    skip_whitespace(r);
    opt = r+1;

  } while(*r == ',');

  if (*r == '\0') return PARS_OK;
  return PARS_SYNTAX_ERROR;
}
