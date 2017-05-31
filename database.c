#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "mpc.h"
#include "database.h"


// Helper for parsing
// XXX: No error checking since if the parse succeeded, the input must be
// valid
static void str_to_entry(client_info* entry, char const* raw_str) {
  char buf[SUBNUM_STRLEN + 1]; // Intermediate buffer for null-termination

  
  // Fill out the entry
  memcpy(buf, raw_str, SUBNUM_STRLEN);
  buf[SUBNUM_STRLEN] = '\0';
  entry->number = strtol(buf, NULL, 10);   

  memcpy(buf, raw_str + SUBNUM_STRLEN, TECH_STRLEN);
  buf[TECH_STRLEN] = '\0';
  entry->ttype = strtol(buf, NULL, 10);   

  entry->paid =
    raw_str[SUBNUM_STRLEN + TECH_STRLEN] == '1' ? true : false;
}


// Helper; for use with bsearch() and qsort()
static int compare_sub_num(void const* info1, void const* info2) {
  return ((client_info*)info1)->number - ((client_info*)info2)->number;
}
  

bool parse_database_file(char const* filename, database* db) {
  // Alloc parsers
  mpc_parser_t* p_database = mpc_new("database");
  mpc_parser_t* p_entry = mpc_new("entry");
  mpc_parser_t* p_paid = mpc_new("paid");
  mpc_parser_t* p_tech = mpc_new("tech");
  mpc_parser_t* p_sub_num = mpc_new("sub_num");
  mpc_parser_t* p_spaces = mpc_new("spaces");
  mpc_parser_t* p_digit = mpc_new("digit");


  // The grammar for the database
  mpc_err_t* err;

  err = mpca_lang(MPCA_LANG_WHITESPACE_SENSITIVE,
    "database : <entry>+ ; "
    "entry : <sub_num> <spaces> <tech> <spaces> <paid> '\n' ; "
    "paid : '1' | '0' ; "
    "tech : <digit><digit> ; "
    "spaces : ' '+ ; "
    "sub_num : <digit> <digit> <digit> '-' <digit> <digit> <digit>"
    " '-' <digit> <digit> <digit> <digit> ; "
    "digit : /[0-9]/ ; ",
    p_database,
    p_entry,
    p_paid,
    p_tech,
    p_sub_num,
    p_spaces,
    p_digit,
    NULL);


  if (err) {
    mpc_err_print(err);
    mpc_err_delete(err);
  }

  // If this assertion fails, then the grammar was specified wrong statically
  assert(err == NULL);


  // Try parsing
  mpc_result_t parse_result;
  size_t db_index = 0; // Current index of entry we're adding

  if (mpc_parse_contents(filename, p_database, &parse_result)) {
    mpc_ast_t* root = (mpc_ast_t*)parse_result.output;
    mpc_ast_trav_t* trav =
      mpc_ast_traverse_start(root, mpc_ast_trav_order_post);


    // Extract info from AST
    char curr_str[BUFSIZ]; // Current raw info string
    char* curr_token = curr_str; // Current location to put the next token
    

    while(trav) {
      // Check for storage overflow
      if (db_index >= MAX_ENTRIES) {
        fprintf(stderr, "parse_database_file: database too large!\n"); 
        return false;
      }


      // Switch on type of node
      if (strstr(trav->curr_node->tag, "entry")) {
        // Process the current read string (i.e. the previous one just
        // finished
        
        // Reset
        *curr_token++ = '\0';
        curr_token = curr_str;
        str_to_entry(&db->entries[db_index], curr_str);
        

        // Move onto next entry
        ++db_index;
      } else if (strstr(trav->curr_node->tag, "digit") 
             || strstr(trav->curr_node->tag, "paid|char")) {
        *curr_token++ = *trav->curr_node->contents;
      }


      // Go to next node...
      mpc_ast_traverse_next(&trav);
    }
              

    // Store number of entries
    db->n_filled = db_index;


    // Cleanup
    mpc_ast_delete((mpc_ast_t*)parse_result.output);

  } else {
    // Bad parse
      mpc_err_print(parse_result.error);
    mpc_err_delete(parse_result.error);
    mpc_cleanup(7, p_database, p_entry, p_paid, p_tech, p_sub_num, p_spaces,
       p_digit);
    return false;
  }


  // Cleanup...
  mpc_cleanup(7, p_database, p_entry, p_paid, p_tech, p_sub_num, p_spaces,
    p_digit);


  // Sort the database
  qsort(db->entries, db_index, sizeof(client_info), &compare_sub_num);


  fprintf(stderr, "Database initialized with:\n");
  dump_database(db);


  return true;
} 


client_info* lookup(database const* db, subscriber_num num) {
  client_info info; // Dummy key for search
  info.number = num;


  return (client_info*)bsearch(&info, db->entries, db->n_filled,
    sizeof(client_info), &compare_sub_num);
}


void dump_database(database const* db) {
  fprintf(stderr, "Capacity = %u\n", MAX_ENTRIES);
  fprintf(stderr, "No. of entries = %lu\n", db->n_filled);
  for (size_t i = 0; i < db->n_filled; ++i) {
    fprintf(stderr, "%u, %u, %u\n", db->entries[i].number,
      db->entries[i].ttype, db->entries[i].paid);
  }
}
