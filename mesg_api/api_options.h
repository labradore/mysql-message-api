#include "options.h"

static option_list all_api_options[] = {	/* keep sorted: binary searched */
    { "group"  , MAX_GROUP_NAME, 0, 0 } ,
    { "handle" , MAX_GROUP_NAME, 0, 0 } ,
    { "join"   , MAX_GROUP_NAME, 0, 0 } ,
    { "name"   , MAX_GROUP_NAME, 0, 0 } ,
    { "track"  , MAX_GROUP_NAME, 0, 0 } ,
    { "" , 0, 0, 0 }   /* This marks the end of the list */
};

enum options_ordered {
  OP_group = 0,
  OP_handle,
  OP_join,
  OP_name,
  OP_track,
  N_API_OPTIONS
};

enum options_bit_flags {
  OPF_group     = (1 << OP_group),
  OPF_handle    = (1 << OP_handle),
  OPF_join      = (1 << OP_join),
  OPF_name      = (1 << OP_name),
  OPF_track     = (1 << OP_track)
};

