#include <stdio.h>
#include <time.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "client.h"
#include "packet.h"
#include "busywait.h"


bool client_init(client* cl, struct sockaddr_in const* addr, client_id id) {
  memset(cl, 0, sizeof(client));


  // Initialize address
  if (addr == NULL) {
    cl->addr.sin_family = AF_INET;
    cl->addr.sin_port = htons(0); // Let bind() choose a random port
    cl->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    memcpy(&cl->addr, addr, sizeof(struct sockaddr_in));
  }

  
  // Set timeout time
  //cl->timeout.tv_sec = TIMEOUT; // FIXME reinstate!!
  cl->timeout.tv_sec = 1;
  cl->timeout.tv_usec = 0;


  // Set ID
  cl->id = id;


  // Setup the socket
  if ((cl->sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("client_init: Couldn't create socket!");
    return false;
  }

  if (bind(cl->sock_fd,
      (struct sockaddr*)&cl->addr, sizeof(struct sockaddr_in)) < 0) {
    perror("client_init: Couldn't bind address to socket!");
    return false;
  }


  return true;
}


void client_send_packet(client* cl, packet_info const* pi, struct sockaddr_in const* dest) {
  size_t raw_len; // Length of data to send
  packet_info reply_pi; // Packet data of any replies (ACKs)


  // Serialize packet data
  raw_len = flatten(pi, cl->send_buf, sizeof(cl->send_buf));

  
  // Send
  if (sendto(cl->sock_fd, cl->send_buf, raw_len, 0,
           (struct sockaddr*)dest, sizeof(struct sockaddr_in)) == -1) {
    perror("client_send_packet: Failed to send!");
    return;
  }

  
  // Start timer and wait for ACK if data was sent
  sequence_num seq_num = pi->cont.data_info.seq_num; // Alias for readability

  while (cl->tries[seq_num] < MAX_TRIES) {
    memset(&reply_pi, 0, sizeof(reply_pi)); // For debugging... 
    memset(&cl->recv_buf, 0, sizeof(cl->recv_buf)); // For debugging... 

    // Wait...then check if timed out
    if (!client_recv_packet(cl)) {
      // Timed out; try again
      fprintf(stderr, "client_send_packet: Timed out waiting for ACK or reply!\n");
      ++cl->tries[seq_num];


    } else if (
        interpret_packet(cl->recv_buf, &reply_pi, sizeof(cl->recv_buf)) != 0
      ) {
      // Invalid packet
      fprintf(stderr,
        "client_send_packet: Received invalid reply! Retrying..\n");

      continue;

    } else {
      fprintf(stderr, "client_send_packet: Received reply of %lu bytes\n",
        cl->last_recvd_len);

      if (!client_handle_reply(cl, &reply_pi)) {
        return;
      }
    }
  }
  

  // Max tries reached; give up and reset info for that sequence number
  fprintf(stderr,
    "client_send_packet: Maximum tries reached! Giving up.\n");

  cl->tries[seq_num] = 0;
}


bool client_handle_reply(client* cl, packet_info const* reply_pi) {
  (void)cl; // XXX: turns out client's not needed, but it may well be,
            // so kept the param for consistency


  switch(reply_pi->type) {
    case REJECT:  
      fprintf(stderr, "client_send_packet: Received REJECT message!");
      alert_reject(reply_pi, reply_pi->cont.reject_info.code);

      return false;

    case ACK:
      // Got an ACK after all
      fprintf(stderr,
        "client_send_packet: Got an ACK for sequence number: %u\n",
        reply_pi->cont.data_info.seq_num);
      

      return true;

    case NOT_EXIST:
    case NOT_PAID:
    case ACC_OK:
      alert_reply(reply_pi);
      return false;
      

    default:
      fprintf(stderr,
        "client_send_packet: Received non-ACK, non-REJECT packet!\n");
  }

  return true;
}


bool client_recv_packet(client* cl) {
  DEFINE_ARGS(recv, sargs,
    cl->sock_fd,
    cl->recv_buf,
    sizeof(cl->recv_buf),
    MSG_DONTWAIT // To return immediately when no data's there
  );


  return busy_wait_until(cl->timeout, &try_recv, &sargs, &cl->last_recvd_len);
}
