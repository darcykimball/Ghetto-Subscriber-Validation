#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H


#include "shell.h"
#include "client.h"


#define N_COMMANDS 2


// The single client instance
extern client the_client;


// Command table
extern const command_pair commands[];


//
// Client shell commands
//


// Send an access request
void send_req(size_t argc, char** argv);


// Dump the current client configuration
// Usage: dump_config
void dump_config(size_t, char**);


#endif // CLIENT_COMMANDS_H
