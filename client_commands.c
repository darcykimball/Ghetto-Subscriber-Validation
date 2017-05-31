#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include "client_commands.h"
#include "packet.h"
#include "shell.h"
#include "client.h"
#include "database.h"
#include "raw_iterator.h"


// Command map
const command_pair commands[] = {
  { "dump_config", &dump_config },
  { "send_req", &send_req },
};


// Helper for parsing IP/port/sequence number
static bool parse_ip_args(char** argv, struct sockaddr_in* dest_addr, sequence_num* seq_num) {
  char* end = NULL; // For parsing port number
  struct addrinfo* aip; // For parsing address/port
  int gai_retval; // For return value of getaddressinfo()


  // XXX: not safe if IPv6...
  if ((gai_retval = getaddrinfo(argv[1], argv[2], NULL, &aip))) {
    if (gai_retval == EAI_SYSTEM) {
      perror("parse_ip_args");
    }

    fprintf(stderr, "parse_ip_args: %s\n", gai_strerror(gai_retval));
    return false;
  }

  assert(aip->ai_addrlen == sizeof(struct sockaddr_in));

  *dest_addr = *(struct sockaddr_in*)(aip->ai_addr);
  freeaddrinfo(aip);


  // Validate seq_num param
  *seq_num = strtoul(argv[3], &end, 10);
  if (end == argv[3] || *end != '\0') {
    SHELL_ERROR("parse_ip_args: Invalid sequence number!");
    return false;
  }


  return true;
}


// Helper for parsing subcriber number/tech type
static bool parse_client_info(client_info* info, char const* num_str, char* const tech_str) {
  char* end; // For use with strtol()


  if (strlen(num_str) != SUBNUM_STRLEN) {
    SHELL_ERROR("parse_client_info: Bad subscriber number length!\n");
    return false;
  }


  if (strlen(tech_str) != TECH_STRLEN) {
    SHELL_ERROR("parse_client_info: Bad technology type length!\n");
    return false;
  }


  info->number = strtoul(num_str, &end, 10);
  if (end == num_str || *end != '\0') {
    SHELL_ERROR("parse_ip_args: Invalid subscriber number!");
    return false;
  }


  info->ttype = strtoul(tech_str, &end, 10);
  if (end == tech_str|| *end != '\0') {
    SHELL_ERROR("parse_ip_args: Invalid technology type!");
    return false;
  }


  return true;
}


// TODO
void send_req(size_t argc, char** argv) {
  struct sockaddr_in dest_addr; // To hold the destination address
  packet_info pi; // For building the packet to send
  sequence_num seq_num; // User-supplied sequence number; usually not needed
                        // but in this allows explicit out-of-sequence sends
  char ip_str[INET_ADDRSTRLEN]; // For message printing
  client_info info; // For storing user input
  uint8_t payload[sizeof(subscriber_num) + sizeof(tech_type)]; // Payload to send
  raw_iterator rit; // For setting up payload


  // Check for and validate arguments
  if (argc != 6) {
    SHELL_ERROR("Usage: send_req [dest_ip] [port] [seq_num] [sub_num] [tech_type]");
    return;
  }

  if (!parse_ip_args(argv, &dest_addr, &seq_num)
        || !parse_client_info(&info, argv[4], argv[5])) {
    return;
  }
  
   
  // Construct the packet
  info.number = htonl(info.number);

  rit_init(&rit, payload, sizeof(payload));
  rit_write(&rit, sizeof(tech_type), &info.ttype);
  rit_write(&rit, sizeof(subscriber_num), &info.number);
  
  pi.type = ACC_PER;
  pi.id = the_client.id;
  pi.cont.data_info.seq_num = seq_num;
  pi.cont.data_info.len = sizeof(payload);
  pi.cont.data_info.payload = payload;

  
  // Send
  fprintf(stderr, "Sending to IP %s, port %u\n",
    inet_ntop(AF_INET, &dest_addr.sin_addr, ip_str, INET_ADDRSTRLEN),
    ntohs(dest_addr.sin_port));

  client_send_packet(&the_client, &pi, &dest_addr);
}


void dump_config(size_t argc, char** argv) {
  // Unused params
  (void)argc;
  (void)argv;


  char ip_str[INET_ADDRSTRLEN];

  fprintf(stderr,
    "Client IP: %s\nClient Port: %u\nClient ID: %u Client Socket: %d\n",
      inet_ntop(AF_INET, &the_client.addr.sin_addr.s_addr, ip_str,
        INET_ADDRSTRLEN),
      ntohs(the_client.addr.sin_port),
      the_client.id,
      the_client.sock_fd);
}
