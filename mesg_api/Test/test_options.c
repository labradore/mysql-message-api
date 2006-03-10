#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"

CHARSET_INFO my_charset_latin1 ;

void iterate(option_list *opts, int sz) {
  int i;
  char *p;
  
  p = malloc(512);

  for( i = 0 ; i < sz ; i++) {
    strcpy(p,"");
    strncat(p, opts[i].value, opts[i].value_len);
    printf("       %-10s -> %-15s (%d)\n",opts[i].option, p, opts[i].value_len);
  }   
}


int main() {
  int nopts = 3;
  opt_parser_return r;

  option_list Options[] = {	/* keep sorted: binary searched */
    { "group" , OPT_GROUP ,   0, 0 } ,
    { "handle" , OPT_HANDLE , "12", 2 },
    { "name" , OPT_NAME ,     0, 0 } ,
  };

  char *test[] = {
    "name = fred" ,
    "name = joe, handle=white",
    "name=joe,handle=blue",
    "NAME = joseph",
    "name = !!(*$&^&%*",
    "group = orders , name=bob,handle=12",
    "=",
    " , ",
    "= ,",
    "rollercoaster = thunder road",
    ",,==,",
    "name ",
    "  name",
    " = kpe",
    "GROUP = AAA, name",
    "group = mimoo =",
    "@(#*%&^A", 
    NULL
  };
  int i, j;
  
  for(i = 0 ; test[i] ; i++) {
    r = parse_options(nopts, Options, test[i]) ;
    printf("%-40s -- %sparsed \n", test[i], (r ? "not " : ""));
    if(r == PARS_OK)
      iterate(Options, nopts);
    for(j = 0 ; j < nopts ; j++) {
      Options[j].value = 0;
      Options[j].value_len = 0;
    }
  }
}
