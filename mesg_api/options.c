#include "options.h"
#include <udf_spread.h>
#include <my_sys.h>

extern CHARSET_INFO my_charset_latin1;

int binsearch(char *word, int len, option_list *kp, int n) {
	int cond, low, mid, high;

	low = 0;
	high = n - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if ((cond = strncasemp(word, kp[mid].option,len)) < 0)
			high = mid - 1;
		else if (cond > 0)
			low = mid + 1;
		else
			return mid;
	}
	return -1;
}


opt_parser_return parse_options( int n_opts, option_list *opts, char *s) {

  char *opt, *val, *r;
  int i, j;
  int optidx;

  opt = s;

  while(*opt) {
    i = j = 0 ;

    /* skip whitespace */
    for ( ; *opt && my_isspace(&my_charset_latin1,*opt) ; opt++);

    /* the first word is the option name, extending from "p" for length "i" */
    for( val = opt; *val && my_isvar(&my_charset_latin1,*val) ; val++, i++);	

    /* length zero means there was a special character in the wrong place */
    if(i == 0)		return PARS_SYNTAX_ERROR;
    
    /* The option is the string of length "i" beginning at "opt" */
    optidx = binsearch(opt, i, opts, n_opts);
    if(optidx == -1) return PARS_ILLEGAL_OPTION;
    
    /* skip more whitespace */
    for( *val && my_isspace(&my_charset_latin1,*val) ; val++ );
    
    /* Next, there must be an equals sign */
    if(*val != '=')	return PARS_SYNTAX_ERROR;
    
    /* Skip more whitespace */
    for( val++ ; *val && my_isspace(&my_charset_latin1,*val); val++ );

    /* Everything until the next NULL or comma or whitespace is the value */ 
    for( r = val ; *r && *r != ',' && (! my_isspace(&my_charset_latin1,*r)) ; r++, j++);

    /* The value is the string of length "j" beginning at "val" */
    opts[optidx].value = val;
    opts[optidx].vale_len = j;
    
    /* Skip more whitespace */
    for( *r++ ; *r && my_isspace(charset_info,*r); r++ );
  
    /* Keep going? */
    if(*r != ',') break;
  }
  
  return PARS_OK;
}
    
