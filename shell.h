#ifndef SHELL_H
#define SHELL_H


#include <stdlib.h>
#include <stdbool.h>

#define MAX_CMD_LEN 16

#define SHELL_ERROR(errmsg) fprintf(stderr, "%s\n", (errmsg));


extern bool echo; // Flag for echoing commands entered


typedef void (*command_fn)(size_t argc, char** argv);


typedef struct {
  char command_str[MAX_CMD_LEN]; // Name of command
  command_fn fn; // Command to be invoked 
} command_pair;


// Main interpreter loop
void loop(command_pair const* command_map, size_t n_commands);


#endif // SHELL_H
