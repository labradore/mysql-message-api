/* Copyright (C) 2006 MySQL AB

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

This product uses software developed by Spread Concepts LLC for use in the 
Spread toolkit. For more information about Spread see http://www.spread.org
*/

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

