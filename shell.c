#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for getline()
#endif

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "shell.h"


#define MAX_N_TOKENS 100


bool echo = 0; // Echo off by default


static char const PROMPT_STR[] = ">>";


static size_t tokenize(char* str, char** tokens, size_t max_tokens);
static command_fn lookup(command_pair const* commands, size_t n_commands,
  char const* command);  


void loop(command_pair const* command_map, size_t n_commands) {
  char* input = NULL; // Buffer to hold raw line input
  size_t line_len = 0; // Number of characters in read line
  char* tokens[MAX_N_TOKENS]; // To hold tokens for a given line
  size_t n_tokens; // Number of tokens for a given line
  command_fn cmd_fn; // Command to invoke
  bool interactive = isatty(fileno(stdin)); // Is this getting input interactively?

  if (!interactive) {
    echo = true;
  }

  // Main loop
  while (1) {
    // Print prompt
    if (interactive) {
      printf("%s", PROMPT_STR);
    }

    // Get input
    if (getline(&input, &line_len, stdin) == -1) {
      // Error reading or EOF
      break;
    }

    // Tokenize
    n_tokens = tokenize(input, tokens, MAX_N_TOKENS);

    // Special case: user's spamming enter key
    if (n_tokens == 0) {
      continue;
    }

    // Echo if on
    if (echo) {
      printf("Your command was:\n");
      for (size_t i = 0; i < n_tokens; i++) {
        printf("%s ", tokens[i]);
      }
      printf("\n");
    }

    // Lookup the command
    if ((cmd_fn = lookup(command_map, n_commands, tokens[0])) == NULL) {
      printf("Error: command %s not found.\n", tokens[0]);
    } else {
      // Invoke the command
      cmd_fn(n_tokens, tokens);
    }
  }

  // Free buffer alloc'd by getline()
  free(input);

  // Prettier.
  printf("\n");
}


// Split input string on whitespace, setting 'tokens' to contain the array
// of token strings. Returns the number of tokens.
static size_t tokenize(char* str, char** tokens, size_t max_tokens) {
  char* token; // Token returned by strtok() 
  size_t token_count = 0; // Number of tokens consumed so far
  char* strarg = str; // Argument to strtok() must be 'str' on first call,
                      // NULL after

  // Consume as many tokens as possible
  while ((token = strtok(strarg, " \n")) != NULL) {
    // Setup argument for subsequent calls
    if (token_count == 0) {
      strarg = NULL;
    }

    // Check for space
    if (token_count < max_tokens) {
      tokens[token_count] = token;
    } else {
      // No space left
      break;
    }

    token_count++;
  }

  return token_count;
}


// Look up a command; returns NULL if not present
static command_fn lookup(command_pair const* commands, size_t n_commands,
  char const* command) {
  // Look through available commands one-by-one
  for (size_t i = 0; i < n_commands; i++) {
    // Compare input with current command in map
    if (strcmp(commands[i].command_str, command) == 0) {
      // Found it
      return commands[i].fn;
    }
  } 

  // Not found
  return NULL;
}
