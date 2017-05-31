#ifndef CLIENT_H
#define CLIENT_H


#include <stdio.h>
#include <time.h>
#include <netinet/ip.h>

#include "packet.h"


#define MAX_SEQ_NUM  0xFF // Maximum sequence number
#define MAX_TRIES    3    // Maximum times to try sending
#define TIMEOUT      3    // Amount of time, in seconds, to wait for ACK


// Struct for state of client
typedef struct {
  size_t tries[MAX_SEQ_NUM]; // Mapping from packet seq. nums to
                             // number of attempts so far
  int sock_fd; // The socket to use for sending
  struct sockaddr_in addr; // The client's internet address
  struct timeval timeout; // How long wait for an ACK each try
  uint8_t send_buf[BUFSIZ]; // Buffer for preparing packets to send
  uint8_t recv_buf[BUFSIZ]; // Buffer for received packets
  client_id id;             // The client's ID
  size_t last_recvd_len;    // Length of last received reply
} client;


// Client initialization
// Args:
//   cl - The client object
//   addr - A desired address to use; if NULL, INADDR_ANY is used 
//   id - The client's ID
// Postcondition: The passed-in client is initialized with socket/address/timers
// Return value: true if everything was OK, false if not able to initialize
bool client_init(client* cl, struct sockaddr_in const* addr, client_id id);


// Send a packet using the specified socket, timing out and retrying as
// necessary
void client_send_packet(client* cl, packet_info const* pi, struct sockaddr_in const* dest);


// Handle a server response, ACK, REJECT, or otherwise
// Return value: True if more recv()s should be done, false if done processing
bool client_handle_reply(client* cl, packet_info const* reply_pi);


// Receive with packets with timeout
bool client_recv_packet(client* cl);

  
#endif // CLIENT_H
