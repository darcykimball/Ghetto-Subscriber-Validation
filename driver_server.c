#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "shell.h"
#include "server.h"


int main(int argc, char** argv) {
  server serv; // The unique server instance


  // Get the database filename
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [database.txt]\n", argv[0]);
    exit(1);
  }


  // Setup the server
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(DEFAULT_PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  
  if (!server_init(&serv, &serv_addr, argv[1])) {
    fprintf(stderr, "Failed to initialize server! Exiting...\n");
    exit(1);
  }


  // Run...
  server_run(&serv);


  return EXIT_SUCCESS;
}
