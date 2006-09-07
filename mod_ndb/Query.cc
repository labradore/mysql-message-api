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
*/

#include "mod_ndb.h"


/* There are many varieties of query: 
   read, insert, update, and delete (e.g. HTTP GET, POST, and DELETE);
   single-row lookups and multi-row scans;
   access via primary key, unique hash index, ordered index, or scan filter;
   queries that use blobs;
   and final results returned in several different formats.
   Each variant possibility is represented as a "module" in this file, 
   and most (but not all) of those modules are functions that conform
   to the "PlanMethod" typedef.
*/
typedef int PlanMethod(request_rec *, config::dir *, struct QueryItems *);


/* A runtime column is the "other half" of the config::key_col structure
*/
struct runtime_col {
  char *value;
  int ndb_col_id;
  short next;
};

/* The main Query() function has a single instance of the QueryItems structure,
   which is used to pass essential data among the modules  
*/
struct QueryItems {
  const NdbDictionary::Table *tab;
  const NdbDictionary::Index *idx;
  NdbTransaction::ExecType ExecType;
  NdbOperation *op;
  NdbIndexScanOperation *scanop;
  NdbBlob *blob;
  NdbRecAttr **result_cols;
  struct runtime_col *keys;
  short active_index;
  short key_columns_used;
  AccessPlan plan;
  table *form_data;
  PlanMethod *op_setup;
  PlanMethod *op_action;
  PlanMethod *send_results;
};  

/* Most modules are represented as PlanMethods
*/
namespace Plan {
 PlanMethod SetupRead; PlanMethod SetupWrite; PlanMethod SetupDelete; // setups
 PlanMethod Read;      PlanMethod Write;      PlanMethod Delete;      // actions
};  

/*  Result formatters:
*/
 PlanMethod Results_none;    PlanMethod Results_JSON;
 PlanMethod Results_raw;     PlanMethod Results_XML;
 PlanMethod Results_ap_note;

/* This array corresponds to the four items in enum result_format:
*/
PlanMethod *result_formatter[5] = { 
  Results_none , Results_JSON , Results_raw, Results_XML, Results_ap_note
};


/* Utility function declarations
*/
enum ValueOp {  equal, setValue, setBound };
int mval_ndb_operation(request_rec *, struct QueryItems *, 
                       const NdbDictionary::Column *, ValueOp, mvalue);
short key_col_bin_search(char *, config::dir *);


/* Some very simple modules are fully defined here:
*/
int Plan::SetupRead(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->plan == OrderedIndexScan ? 
    q->scanop->readTuples(NdbOperation::LM_Read) :
    q->op->readTuple(NdbOperation::LM_Read);
}

int Plan::SetupWrite(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->writeTuple(); 
}

int Plan::SetupDelete(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->deleteTuple(); 
}



/* set_key() is the inlined code which processes both pathinfo 
   and request params, sets the items in the Q.keys array, and
   determines the access plan based on the indexes encountered
   in the query parameters
*/
inline void set_key(request_rec *r, short n, char *value, config::dir *dir, 
                    struct QueryItems *q) 
{
  const NdbDictionary::Column *column;
  const config::key_col &keycol = dir->key_columns->item(n);
  
  q->keys[n].value = value; 
  column = q->tab->getColumn(keycol.name);
  if(column) {
    q->keys[n].ndb_col_id = column->getColumnNo();
    log_debug3(r->server, "Config column %s = NDB AttrID %d", 
               keycol.name, q->keys[n].ndb_col_id);
  }
  else return; 
  q->key_columns_used++;
 
  if(keycol.implied_plan > q->plan) {
    q->plan = keycol.implied_plan;
    q->active_index = keycol.index_id;
  }
}

// =============================================================


/* Query():
   Process an HTTP request,
   formulate and run an NDB execution plan,
   and create a results page.
*/
int Query(request_rec *r, config::dir *dir, ndb_instance *i) 
{
  const NdbDictionary::Dictionary *dict;
  struct QueryItems Q = 
    { 0, 0, NdbTransaction::NoCommit, 0,0,0,0,0,0,0, NoPlan, 0, 
      Plan::SetupRead, Plan::Read, 0 };
  int response_code;
  short col;
  ValueOp mval_op = equal;
  mvalue mval;
  const NdbDictionary::Column *ndb_Column;

  /* Initialize the data dictionary 
  */
  i->db->setDatabaseName(dir->database);
  dict = i->db->getDictionary();
  Q.tab = dict->getTable(dir->table);
  if(Q.tab == 0) { 
    ap_log_error(APLOG_MARK, log::err, r->server,
                 "mod_ndb could not find table %s (in database %s) "
                 "in NDB Data Dictionary: %s",dir->table,dir->database,
                 dict->getNdbError().message);
    i->errors++;
    return NOT_FOUND; 
  }
  
  
  /* Initialize Q.keys, the runtime array of key columns which is used
     in parallel with the configure-time array dir->key_columns
  */
  Q.keys = (struct runtime_col *) ap_pcalloc(r->pool, 
           (dir->key_columns->size() * sizeof(struct runtime_col)));


  /* Many elements of the query plan depend on the HTTP operation --
     GET, POST, or DELETE.  Set these up here.
  */
  switch(r->method_number) {
    case M_GET:
      // Allocate an array of NdbRecAttrs for all desired columns 
      Q.result_cols = (NdbRecAttr **) 
        ap_pcalloc(r->pool, dir->visible->size() * sizeof(NdbRecAttr *));
      Q.send_results = result_formatter[dir->results];
      break;
    case M_POST:
      Q.op_setup = Plan::SetupWrite;
      Q.op_action = Plan::Write;
      Q.ExecType = NdbTransaction::Commit;
      Q.send_results = Results_none;
      Q.form_data = 0;
      /* Fetch the update request from the client */
      response_code = read_http_post(r, & Q.form_data);
      if(response_code != OK) return response_code;
      break;
    case M_DELETE:
      if(! dir->allow_delete) return DECLINED;
      Q.op_setup = Plan::SetupDelete;
      Q.op_action = Plan::Delete;
      Q.ExecType = NdbTransaction::Commit;
      Q.send_results = Results_none;
      break;
    default:
      return DECLINED;
  }


  /* ===============================================================
     Process pathinfo, and then arguments.  Determine an access plan.
     The detailed work is done within the inlined function set_key().
  */

  /* Pathinfo.  Process r->path_info from right to left. */
  if(dir->pathinfo_size) {
    size_t item_len = 0;
    short element = dir->pathinfo_size - 1;
    register const char *s;
    // Set s to the end of the string, then work backwards.
    for(s = r->path_info ; *s; ++s);
    if(* (s-1) == '/') s -=2;   /* ignore a trailing slash */
    for(; s >= r->path_info && element >= 0; --s) {
      if(*s == '/') {
        set_key(r, dir->pathinfo[element--], 
                ap_pstrndup(r->pool, s+1, item_len), 
                dir, &Q);
        item_len = 0;
      }
      else item_len++;
    }
  }
  
  /* Arguments */
  if(r->args) {
    register const char *c = r->args;
    char *key, *val;
    short n;
      
    while(*c && (val = ap_getword(r->pool, &c, '&'))) {
      key = ap_getword(r->pool, (const char **) &val, '=');
      ap_unescape_url(key);
      ap_unescape_url(val);
      n = key_col_bin_search(key, dir);
      if(n >= 0) { 
        set_key(r, n, val, dir, &Q);
      }
    }
  }  
  /* ===============================================================*/
  if(! Q.key_columns_used) return NOT_FOUND;    /* no query */

  /* Open a transaction.
     This creates an obligation to close it later, using tx->close().
  */    
  if(!(i->tx = i->db->startTransaction())) {
    log_err2(r->server,"db->startTransaction failed: %s",
              i->db->getNdbError().message);
    return NOT_FOUND;
  }

  /* Now set the Query Items that depend on the access plan.
  */
  if(Q.plan == PrimaryKey) {
    Q.op = i->tx->getNdbOperation(Q.tab);
    log_debug(r->server,"Using primary key lookup; key %d",Q.active_index);
  }
  else {                                        /* Indexed Access */
    register const char * idxname = dir->indexes->item(Q.active_index).name;
    Q.idx = dict->getIndex(idxname, dir->table);
    if(! Q.idx)
    {
      ap_log_error(APLOG_MARK, log::err, r->server, "mod_ndb: index %s "
                   "does not exist (db: %s, table: %s)", idxname, 
                   dir->database, dir->table);
      goto abort;
    }    
    if(Q.plan == UniqueIndexAccess) {
      log_debug3(r->server,"Using UniqueIndexAccess; key # %d - %s",
                 Q.active_index, Q.idx->getName());
      Q.op = i->tx->getNdbIndexOperation(Q.idx);
    }
    else if(Q.plan == OrderedIndexScan) {
      log_debug3(r->server,"Using OrderedIndexScan; key # %d - %s",
                 Q.active_index, Q.idx->getName());
      Q.op = Q.scanop = i->tx->getNdbIndexScanOperation(Q.idx);
      mval_op = setBound;
    }
    else
      log_debug(r->server," --SHOULD NOT HAVE REACHED THIS POINT-- %d",Q.plan);
  }
  if(Q.active_index < 0) return NOT_FOUND;


  // Query setup, e.g. Plan::SetupRead calls op->readTuple() 
  if(Q.op_setup(r, dir, & Q)) { // returns 0 on success
    log_debug(r->server,"Returning 404 because Q.op_setup() failed: %s",
              Q.op->getNdbError().message);
    goto abort;
  }


  // Traverse the index parts and build the query
  col = dir->indexes->item(Q.active_index).first_col;
  ndb_Column = Q.tab->getColumn(Q.keys[col].ndb_col_id);

  while (col >= 0 && Q.key_columns_used-- > 0) {
    log_debug3(r->server," ** Query key: %s -- value: %s", 
               dir->key_columns->item(col).name, Q.keys[col].value);
    
    mval = MySQL::value(r->pool, ndb_Column, Q.keys[col].value);
    if(mval_ndb_operation(r, & Q, ndb_Column, mval_op, mval) != OK)
      goto abort;
    
    col = dir->key_columns->item(col).next_in_key;
  }

  // To do: set filters

  // Perform the action; i.e. get the value of each column
  response_code = Q.op_action(r, dir, &Q);
  
  if(response_code == OK) {
    /* Execute the transaction.  tx->execute() returns 0 on success. */
    if(i->tx->execute(Q.ExecType, NdbTransaction::AbortOnError, 
                      i->conn->ndb_force_send))
    {        
      log_debug(r->server,"Returning 404 because tx->execute failed: %s",
                i->tx->getNdbError().message);
      response_code = NOT_FOUND;
    }
    else Q.send_results(r, dir, & Q);
  }
  
  i->tx->close();
  return response_code;
  
  abort:
  i->tx->close();
  return NOT_FOUND;
}



/******** Result Page formatters *************/

int Results_none(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_none");
  return 0;
}


inline void JSON_send_result_row(request_rec *r, config::dir *dir, 
                                 struct QueryItems *q) {
  ap_rputs(JSON::new_object,r);
  for(int n = 0; n < dir->visible->size() ; n++) {
    if(n) ap_rputs(JSON::delimiter,r);
    ap_rputs(JSON::member(*q->result_cols[n],r),r);
  }
  ap_rputs(JSON::end_object,r);
}
  

int Results_JSON(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_JSON");
  ap_send_http_header(r); 
  if(q->scanop) {
    int nrows = 0, check;
    /* Multi-row result set */
    ap_rputs(JSON::new_array,r);
    while((check = q->scanop->nextResult(true)) == 0) {
      log_debug(r->server,"Results_JSON getting a batch of rows (row %d)", nrows);
      do {
        if(nrows++) ap_rputs(JSON::delimiter,r);
        JSON_send_result_row(r, dir, q);
      } while((check = q->scanop->nextResult(false)) == 0);
    }
    ap_rputs(JSON::end_array,r);
    log_debug(r->server,"Results_JSON check = %d", check);
  }
  else /* Single row result set */
    JSON_send_result_row(r, dir, q);

  return 0;
}

int Results_raw(request_rec *r, config::dir *dir, struct QueryItems *q) {
  unsigned long long size64 = 0;
  unsigned int size = 0;
  void *buffer;
  
  if(q->blob) {
    q->blob->getLength(size64);  //passed by reference
    size = (unsigned int) size64;
    buffer = ap_palloc(r->pool, size);
    if(q->blob->readData(buffer,size)) { // 0 on success
      log_debug(r->server,"Error reading blob data: %s",
                q->blob->getNdbError().message);
    }
    ap_set_content_length(r, size);
    ap_send_http_header(r); 
    ap_rwrite(buffer, size, r);
  }
  return 0;
}

int Results_XML(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_XML");

  return 0;
}

int Results_ap_note(request_rec *r, config::dir *dir, struct QueryItems *q) {
  
  log_debug(r->server,"In Results formatter %s", "Results_note");
  for(int n = 0; n < dir->visible->size() ; n++) {
    register NdbRecAttr *rec =  q->result_cols[n];
    if(! rec->isNULL()) {
      register const NdbDictionary::Column* col = rec->getColumn();
      ap_table_set(r->notes, col->getName(), MySQL::result(r->pool, *rec));
    }
  }
  return 0;
}

/* ===============================================================*/



/* mval_ndb_operation() is a wrapper around 
   NdbOperation->equal() and ->setValue(),
   which takes mvalues (from MySQL_Field.h),
   logs some errors and debug messages, 
   and returns an HTTP status code.
*/

int mval_ndb_operation(request_rec *r, struct QueryItems *q, 
                   const NdbDictionary::Column *col, ValueOp op, mvalue mval)
{
  if(mval.use_value == can_not_use) {
    ap_log_error(APLOG_MARK, log::err, r->server,
                 "Cannot use MySQL column %s in query -- data type "
                 "not supported by mod_ndb",mval.u.err_col->getName());
    return NOT_FOUND;
  }

  if(mval.use_value == use_char) /* delete this after it works properly */
    log_debug(r->server,"mval.u.val_char: %s", mval.u.val_char);
  
  switch(op) {  // crazy NDB bug here!  Should use col->getColumnNo() instead of getName() !
    case equal:
      if(!(q->op->equal(col->getName(),                       // 0 on succes
                        reinterpret_cast<const char *> (&mval.u.val_char) )))
        return OK;
    case setValue:
      if(!(q->op->setValue(col->getName(),                   // 0 on success
                           reinterpret_cast<const char *> (&mval.u.val_char) ))) 
        return OK;
    case setBound:
      if(!(q->scanop->setBound(col->getName(), NdbIndexScanOperation::BoundEQ,
                               &mval.u.val_signed))) 
        return OK;
    default:
      log_debug(r->server," --SHOULD NOT HAVE REACHED THIS POINT-- %d",op);
      return NOT_FOUND;
  }
    log_debug3(r->server,"op->x (%d)  failed: %s", op, q->op->getNdbError().message);
    return NOT_FOUND;
}


int Plan::Read(request_rec *r, config::dir *dir, struct QueryItems *q) {  
  char **column_list;

  // Call op->getValue() for each desired result column
  column_list = dir->visible->items();
  for(int n = 0; n < dir->visible->size() ; n++) {
    q->result_cols[n] = q->op->getValue(column_list[n], 0);

    /* If the result format is "raw", check for BLOBs */
    if(dir->results == raw) {
      int isz = q->tab->getColumn(column_list[n])->getInlineSize();
      if(isz) {   /* then the column is a blob... */
        log_debug(r->server,"Treating column %s as a blob",column_list[n])
        q->blob = q->op->getBlobHandle(column_list[n]);
      }
    }
  }
  return OK;
}


int Plan::Write(request_rec *r, config::dir *dir, struct QueryItems *q) {
  char **column_list;
  const NdbDictionary::Column *col;
  mvalue mval;
  const char *key, *val;
  
  column_list = dir->updatable->items();
  // iterate over the updatable columns
  for(int n = 0; n < dir->updatable->size() ; n++) {
    key = column_list[n];
    // finding them in the posted form data
    val = ap_table_get(q->form_data, key);
    if(val) {   
      log_debug(r->server,"Updating column %s",key);
      col = q->tab->getColumn(key);
      if(col) {
        // Encode the HTTP ASCII data into proper MySQL data types
        mval = MySQL::value(r->pool, col, val);
        // And call op->setValue
        mval_ndb_operation(r, q, col, setValue, mval);
      }
      else log_err2(r->server,"AllowUpdate list includes invalid column name %s", key);
    }
  }
  return OK;
}


int Plan::Delete(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"Deleting Row %s","")
  return OK;
}


/* Based on Kernighan's C binsearch from TPOP pg. 31
*/
short key_col_bin_search(char *name, config::dir *dir) {
  int low = 0;
  int high = dir->key_columns->size() - 1;
  int mid;
  register int cmp;
  
  while ( low <= high ) {
    mid = (low + high) / 2;
    cmp = strcmp(name, dir->key_columns->item(mid).name);
    if(cmp < 0) 
      high = mid - 1;
    else if (cmp > 0)
      low = mid + 1;
    else
      return mid;
  }
  return -1;
}

