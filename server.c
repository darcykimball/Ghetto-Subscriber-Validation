#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>

#include "server.h"
#include "packet.h"
#include "database.h"
#include "raw_iterator.h"


bool server_init(server* serv, struct sockaddr_in* addr, char const* filename) {
  char ip_str[INET_ADDRSTRLEN]; // For printing info


  fprintf(stderr, "server_init: Initializing server...\n");


  // Setup a socket
  if ((serv->sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server_init: Couldn't create socket");
    return false;
  }


  // Fill in with default address if needed
  if (addr == NULL) {
    memset(&serv->addr, 0, sizeof(serv->addr));
    serv->addr.sin_family = AF_INET;
    serv->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv->addr.sin_port = htons(DEFAULT_PORT);
  } else {
    memcpy(&serv->addr, addr, sizeof(struct sockaddr_in));
  }


  // Bind the socket
  if ((bind(serv->sock_fd, (struct sockaddr*)&serv->addr,
        sizeof(struct sockaddr_in)) == -1)) {
    perror("server_init: Couldn't bind socket to address");
    return false;
  }


  // Initialize next expected sequence numbers
  memset(serv->expect_recv, 0, sizeof(serv->expect_recv));


  // Read in database
  if (!parse_database_file(filename, &serv->db)) {
    fprintf(stderr, "server_init: Couldn't initialize database!\n"); 
    return false;
  }


  fprintf(stderr, "Done initializing!\n");
  fprintf(stderr, "Server IP: %s\n",
    inet_ntop(AF_INET, &serv->addr.sin_addr, ip_str, INET_ADDRSTRLEN));
  fprintf(stderr, "Server port: %u\n", ntohs(serv->addr.sin_port));


  return true;
}


// Process a received packet
void server_process_packet(server* serv, struct sockaddr_in const* ret) {
  packet_info pi; // For storing interpreted packet
  int code; // For return value of server_check_packet()
  char ip_str[INET_ADDRSTRLEN]; // For pretty-printing addresses


  // Validate
  code = server_check_packet(serv, &pi);


  // Notify of any errors if present
  if (code != 0) {
    alert_reject(&pi, (reject_code)code);
    
    // Reply to client with reject message
    fprintf(stderr, "server_run: Sending REJECT message\n");
    server_send_reject(serv, &pi, ret, (reject_code)code);

  } else { // All is well
    // FIXME: pretty print sub_num, etc.
    fprintf(stderr, "server_run: Received message: \"%s\"\n",
      (char const*)pi.cont.data_info.payload);
    fprintf(stderr, "server_run: from %s:%u\n", inet_ntop(AF_INET,
      &ret->sin_addr, ip_str, INET_ADDRSTRLEN), ntohs(ret->sin_port));


    // Send an ACK, and update next expected sequence number
    fprintf(stderr, "server_run: Sending ACK for sequence number %u\n",
      pi.cont.data_info.seq_num);
    server_send_ack(serv, pi.id, ret);
    ++serv->expect_recv[pi.id];

    
    // Check database and send the appropriate response
    server_handle_req(serv, ret, &pi);
  }
}


void server_handle_req(server* serv, struct sockaddr_in const* ret,
  packet_info const* pi) {
  raw_iterator rit; // For reading request
  subscriber_num num;
  tech_type ttype;
  packet_info reply_pi; // Response to client

  
  // Read in the request info
  rit_init(&rit, serv->recv_buf, sizeof(serv->recv_buf));
  rit.curr += sizeof(PACKET_START) + sizeof(client_id) + sizeof(uint16_t)
    + sizeof(sequence_num) + sizeof(payload_len); // Skip to the payload
  rit_read(&rit, sizeof(ttype), &ttype);
  rit_read(&rit, sizeof(subscriber_num), &num);
  num = ntohl(num);


  // Lookup the subscriber
  fprintf(stderr, "server_handle_req: Looking up %u...\n", num);
  client_info* entry = lookup(&serv->db, num);



  // Check lookup result and setup reply packet
  if (!entry) {
    // Subscriber not found
    reply_pi.type = NOT_EXIST;
    fprintf(stderr, "server_handle_req: Subscriber not found!\n");

  } else if (entry->ttype != ttype) {
    // Nonexistent tech type
    fprintf(stderr, "server_handle_req: Subscriber has no access to tech!\n");
    reply_pi.type = NOT_EXIST;

  } else if (!entry->paid) {
    // Entry's there, but hasn't paid
    fprintf(stderr, "server_handle_req: Subscriber has not paid!\n");
    reply_pi.type = NOT_PAID;

  } else {
     // Subscriber granted access
    fprintf(stderr, "server_handle_req: Subscriber granted access.\n");  
    reply_pi.type = ACC_OK;
  }

  // We're basically echoing back the request data, so the payload can be
  // copied from the request
  reply_pi.id = pi->id;
  reply_pi.cont.data_info.seq_num = pi->cont.data_info.seq_num;
  reply_pi.cont.data_info.len = pi->cont.data_info.len;
  reply_pi.cont.data_info.payload = pi->cont.data_info.payload;


  size_t flattened_len =
    flatten(&reply_pi, serv->send_buf, sizeof(serv->send_buf));



  // Send out the reply
  if (sendto(
      serv->sock_fd,
      serv->send_buf,
      flattened_len,
      0,
      (struct sockaddr*)ret,
      sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, "server_handle_req: Unable to send reply!\n");
  } 
}


void server_send_ack(server* serv, client_id id, struct sockaddr_in const* ret) {
  char ip_str[INET_ADDRSTRLEN]; // For msg printing


  packet_info pi;
  pi.type = ACK;
  pi.id = id;
  pi.cont.ack_info.recvd_seq_num = serv->expect_recv[id];


  size_t flattened_len = flatten(&pi, serv->send_buf, sizeof(serv->send_buf));
  
  fprintf(stderr, "server_send_ack: Sending ACK to %s:%u\n",
    inet_ntop(AF_INET,
    &ret->sin_addr, ip_str, INET_ADDRSTRLEN), ntohs(ret->sin_port));

  ssize_t retval = 
    sendto(
      serv->sock_fd,
      serv->send_buf,
      flattened_len,
      0,
      (struct sockaddr*)ret,
      sizeof(struct sockaddr_in)
    ); 


  if (retval == -1) {
    perror("server_send_ack: Failed to send ACK");
  }
}


void server_send_reject(server* serv, packet_info const* bad_pi,
  struct sockaddr_in const* ret, reject_code code) {
  packet_info pi;
  pi.type = REJECT;
  pi.id = bad_pi->id;
  pi.cont.reject_info.code = code;
  pi.cont.reject_info.recvd_seq_num = bad_pi->cont.data_info.seq_num;


  size_t flattened_len = flatten(&pi, serv->send_buf, sizeof(serv->send_buf));

  ssize_t retval = sendto(
    serv->sock_fd,
    serv->send_buf,
    flattened_len,
    0,
    (struct sockaddr*)ret,
    sizeof(struct sockaddr_in)
  );

  if (retval == -1) {
    perror("server_send_ack: Failed to send ACK");
  }
}


void server_run(server* serv) {
  struct sockaddr_in client_addr; // To store client IP address
  socklen_t addrlen = sizeof(struct sockaddr_in); // For length of client address
  ssize_t n_recvd; // To hold number of bytes received

  memset(&client_addr, 0, sizeof(client_addr));

  
  // Wait...
  fprintf(stderr, "server_run: Waiting for messages...\n");
  while ((n_recvd =
    recvfrom(serv->sock_fd, serv->recv_buf, sizeof(serv->recv_buf), 0,
      (struct sockaddr*)&client_addr, &addrlen))) // XXX: this implicitly exits upon receiving a 0-byte packet!!
  {
    if (n_recvd == -1) {
      perror("server_run: recvfrom() failed");
      continue;
    }


    // Clear buf
    memset(serv->send_buf, 0, sizeof(serv->send_buf));


    fprintf(stderr, "server_run: Got a packet: %ld bytes!\n", n_recvd);


    // Process and reply
    serv->last_recvd_len = n_recvd;
    server_process_packet(serv, &client_addr);
  }
}


int server_check_packet(server* serv, packet_info* pi) {
  // Read and check the packet for errors
  int code = interpret_packet(serv->recv_buf, pi, sizeof(serv->recv_buf));


  // This server only accepts access requests
  if (pi->type != ACC_PER) {
    fprintf(stderr, "server_check_packet: Non access-request received!\n");
    return BAD_TYPE;
  }


  // Check for invalid length field: does the raw packet end on the expected
  // boundary?
  uint8_t* payload_end =
    serv->recv_buf
      + sizeof(PACKET_START)
      + sizeof(client_id)
      + sizeof(uint16_t) // XXX: packet type
      + sizeof(sequence_num)
      + sizeof(payload_len)
      + pi->cont.data_info.len;
  uint8_t* expected_payload_end =
    serv->recv_buf + serv->last_recvd_len - sizeof(PACKET_END);

  
  if (payload_end != expected_payload_end) {
    return BAD_LEN;
  }


  // Check for additional errors
  if (code == 0) {
    // Check sequence number
    // FIXME: special case: both zero -> overflow!...or will it wrap around??
    if (pi->cont.data_info.seq_num != serv->expect_recv[pi->id]) {
      if (pi->cont.data_info.seq_num > serv->expect_recv[pi->id]) {
        return OUT_OF_SEQ;
      } else {
        return DUP_PACK;
      }
    }
  }


  return code; 
}
