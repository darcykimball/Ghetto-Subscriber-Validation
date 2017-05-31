#ifndef SERVER_H
#define SERVER_H


#include <stddef.h>
#include <stdlib.h>
#include <netinet/ip.h>

#include "packet.h"
#include "database.h"


#define DEFAULT_PORT 4321

// Get the size of the range of an unsigned type
// XXX: taken from http://stackoverflow.com/questions/2053843/min-and-max-value-of-data-type-in-c
#define urange(t) ((((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | \
                    (0xFULL << ((sizeof(t) * 8ULL) - 4ULL))) + 1)


// Server state
typedef struct {
  sequence_num expect_recv[urange(client_id)]; // Mapping of clients to next
                                            // expected sequence numbers  
  int sock_fd; // The server's send/recv socket
  struct sockaddr_in addr;  // The server's IP address/port
  uint8_t send_buf[BUFSIZ]; // Buffer for packets to be sent
  uint8_t recv_buf[BUFSIZ]; // Buffer for received packets
  size_t last_recvd_len;    // Size of last received packet (inside recv_buf)
  database db;             // Subscriber database
} server;


// Initialize server
// Args:
//   serv - the server object
//   addr - A desired address to use; if NULL, INADDR_ANY is used, with
//   DEFAULT_PORT
//   filename - The database filename
// Return value: True if initialization was OK, false otherwise.
bool server_init(server* serv, struct sockaddr_in* addr, char const* filename);


// Process a received packet; validate and send ACK as necessary
void server_process_packet(server* serv, struct sockaddr_in const* ret);


// Handle an access request, sending responses as appropriate
void server_handle_req(server* serv, struct sockaddr_in const* ret,
  packet_info const* pi);


// Validate a received packet
// PRECONDITION: Server has just received a packet and has filled member
// recv_buf with its contents
// Return value: 0 if everything was OK; an (castable to reject_code)
// appropriate error code otherwise
int server_check_packet(server* serv, packet_info* pi);


// Send an ACK packet
// Precondition: The server just finished processing a valid received packet
// from 'client'.
// Postcondition: The last received sequence number for the client is
// incremented.
void server_send_ack(server* serv, client_id client, struct sockaddr_in const* ret);


// Send a reject (error) packet
void server_send_reject(server* serv, packet_info const* bad_pi,
  struct sockaddr_in const* ret, reject_code code);


// Run the server, i.e. wait indefinitely for packets, process, and reply with
// ACKs as appropriate.
void server_run(server* serv);


#endif // SERVER_H
