#include <stdio.h>

#include "database.h"


int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [db_file.txt]\n", argv[0]);
    return 1;
  }

  database db;


  parse_database_file(argv[1], &db);

  return 0;
}
