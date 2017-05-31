#ifndef DATABASE_H
#define DATABASE_H


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


// FIXME: write prelude on why we're using qsort/bsearch as opposed to a
// binary tree or hash table?? discuss tradeoffs?


#define MAX_ENTRIES 4096 // FIXME: what's reasonable?
#define SUBNUM_STRLEN 10 // Phone numbers are 10 digits
#define TECH_STRLEN 2    // Tech-types are expected to be two chars
#define PAID_STRLEN 1    // 'Paid' status is either the char '0' or '1'


typedef uint32_t subscriber_num; // Client subscriber number
typedef uint8_t tech_type; // Technology type


typedef struct {
  subscriber_num number;
  tech_type ttype;
  bool paid;
} client_info;


typedef struct {
  client_info entries[MAX_ENTRIES];
  size_t n_filled;
} database;


// Parse database info from a file and initialize an array of the info.
// Params: TODO
// POSTCONDITION: Param 'database' is sorted and contains the parsed info
bool parse_database_file(char const* filename, database* db);  


// Lookup an entry by subscriber number
client_info* lookup(database const* db, subscriber_num num);


// Dump database; for debugging
void dump_database(database const* db);


#endif // DATABASE_H
