#include <stdio.h>
#include <stddef.h>


#include "shell.h"
#include "client.h"
#include "client_commands.h"


client the_client; // The single, global client instance

int main(int argc, char** argv) {
  client_id id; // To be read in as arg


  // Parse cmd line...
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [client_id]\n", argv[0]);
    exit(1);
  }
  id = strtol(argv[1], NULL, 10);


  // Setup the client (configuration)
  if (!client_init(&the_client, NULL, id)) {
    fprintf(stderr, "Unable to start client! Exiting...\n");
    exit(1);
  }


  // Start the shell
  loop(commands, N_COMMANDS);
    

  return EXIT_SUCCESS;
}
