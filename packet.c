#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>

#include "packet.h"
#include "raw_iterator.h"
#include "closure.h"


//
// Constants
//


const unsigned MAX_IOVEC_LEN = 8;
const uint16_t PACKET_START = 0xFFFF;
const uint16_t PACKET_END = 0xFFFF;


size_t flatten(packet_info const* pi, void* buf, size_t size) {
  raw_iterator rit; // For writing to buffer
  rit_init(&rit, (uint8_t*)buf, size);
  size_t flattened_size = 0; // Size of the flattened packet in the buffer
  uint16_t network_short; // For converting shorts to network order
  

  // Add common field sizes to the total packet size
  flattened_size +=
    sizeof(PACKET_START) +
    sizeof(client_id) +
    sizeof(uint16_t) +  // Size of packet_type on the wire
    sizeof(PACKET_END);



  // Header
  rit_write(&rit, sizeof(PACKET_START), &PACKET_START);
  
  
  // Client ID
  rit_write(&rit, sizeof(client_id), &pi->id);


  // Packet type
  network_short = htons(pi->type);
  rit_write(&rit, sizeof(uint16_t), &network_short);


  // XXX: For 1-byte fields, conversion to network order is not necessary;
  // same for start and end flags b/c they'd be the same after conversion
  switch (pi->type) {
    case DATA:
    case ACC_PER:
    case NOT_PAID:
    case NOT_EXIST:
    case ACC_OK:
      // Sequence number
      rit_write(&rit, sizeof(sequence_num), &pi->cont.data_info.seq_num);

      // Data length
      rit_write(&rit, sizeof(payload_len), &pi->cont.data_info.len);

      // Payload
      rit_write(&rit, pi->cont.data_info.len, pi->cont.data_info.payload);

      flattened_size +=
        sizeof(sequence_num) +
        sizeof(payload_len) +
        pi->cont.data_info.len;

      break;
    case ACK:
      // Received sequence number
      rit_write(&rit, sizeof(sequence_num), &pi->cont.ack_info.recvd_seq_num);

      flattened_size += sizeof(sequence_num);

      break;
    case REJECT:
      // Reject code
      network_short = htons(pi->cont.reject_info.code);
      rit_write(&rit, sizeof(uint16_t), &network_short);

      // Received sequence number
      rit_write(&rit, sizeof(sequence_num),
        &pi->cont.reject_info.recvd_seq_num);

      flattened_size +=
        sizeof(uint16_t) + // Reject code size
        sizeof(sequence_num);
              

      break;
    default:
      // Should really be unreachable
      fprintf(stderr, "flatten: Invalid type constant!\n");
      assert(0);
  }

  rit_write(&rit, sizeof(PACKET_END), &PACKET_END);


  return flattened_size;
}


int interpret_packet(void const* buf, packet_info* pi, size_t size) {
  raw_iterator rit; // Raw iterator for reading buffer
  rit_init(&rit, (uint8_t*)buf, size); // XXX: no constness is OK; read only
  uint16_t network_short; // For reading shorts


  // FIXME: remove
#if 0
    fprintf(stderr, "%0x\n", *((uint32_t*)buf));
  fprintf(stderr, " %0x\n", *((uint32_t*)((uint8_t*)buf + 4)));
  fprintf(stderr, " %0x\n", *((uint32_t*)((uint8_t*)buf + 8)));
  fprintf(stderr, " %0x\n", *((uint32_t*)((uint8_t*)buf + 16)));
  fprintf(stderr, " %0x\n", *((uint32_t*)((uint8_t*)buf + 32)));
  
#endif



  // Check header
  // XXX: no network to host conversion needed in this case
  rit_read(&rit, sizeof(uint16_t), &network_short);
  if (network_short != PACKET_START) {
    fprintf(stderr, "interpret_packet: Bad PACKET_START!\n");
    
    // XXX: I overloaded NO_END; see packet.h
    return NO_END;
  }


  // Read client ID
  rit_read(&rit, sizeof(client_id), &pi->id);


  // Read packet type
  rit_read(&rit, sizeof(uint16_t), &network_short);
  pi->type = (packet_type)ntohs(network_short);


  // Interpret based on type
  switch (pi->type) {
    case DATA:
    case ACC_PER:
    case NOT_PAID:
    case NOT_EXIST:
    case ACC_OK:
      // Sequence number
      rit_read(&rit, sizeof(sequence_num), &pi->cont.data_info.seq_num);

      // Payload length
      rit_read(&rit, sizeof(payload_len), &pi->cont.data_info.len);

      // Payload; save ptr to it and skip over
      pi->cont.data_info.payload = rit.curr;
      rit.curr += pi->cont.data_info.len;
        
      break;

    case ACK:
      // Received sequence number
      rit_read(&rit, sizeof(sequence_num), &pi->cont.ack_info.recvd_seq_num);
      
      break;

    case REJECT:
      // Reject code
      rit_read(&rit, sizeof(uint16_t), &network_short);
      pi->cont.reject_info.code = (reject_code)ntohs(network_short);

      // Received sequence number
      rit_read(&rit, sizeof(sequence_num), &pi->cont.reject_info.recvd_seq_num);
      break;

    default:
      // Invalid packet type was found
      fprintf(stderr, "interpret_packet: Bad packet type!\n");
      return BAD_TYPE;
  }


  // Check packet end flag
  rit_read(&rit, sizeof(uint16_t), &network_short);
  if (network_short != PACKET_END) {
    fprintf(stderr, "interpret_packet: Bad PACKET_END!\n");
    return NO_END; 
  }

  
  return 0;
}


bool try_recv(void* args, void* retval) {
  ssize_t n_recvd; // For return value of recv()
  struct STRUCT_ARGS_NAME(recv)* sargs = (struct STRUCT_ARGS_NAME(recv)*)args;

  
  n_recvd = *((ssize_t*)retval) = recv(
      sargs->sockfd,
      sargs->buf,
      sargs->len,
      sargs->flags
  );

  if (n_recvd == -1) {
    //perror("try_recv");
    return false;
  }

  return true;
}


void alert_reject(packet_info const* pi, reject_code code) {
    // Print out errors server-side
    fprintf(stderr, " From client %d: ", pi->id);
    switch (code) {
      case NO_END:
        fprintf(stderr, "No packet terminator\n");
        break;
      case DUP_PACK:
        fprintf(stderr, "Received duplicate\n");
        break;
      case OUT_OF_SEQ:
        fprintf(stderr, "Received out of sequence!\n");
        break;
      case BAD_TYPE:
        fprintf(stderr, "Bad type field!\n");
        break;
      case BAD_LEN:
        fprintf(stderr, "Bad length field!\n");
        break;
      default:
        fprintf(stderr, "Unrecognized reject code...\n");
    }
}

void alert_reply(packet_info const* pi) {
  switch(pi->type) {
    case NOT_PAID:
      fprintf(stderr, "alert_reply: Subscriber has not paid!\n");
      break;
    case NOT_EXIST:
      fprintf(stderr, "alert_reply: Subscriber or technology not found!\n");
      break;
    case ACC_OK:
      fprintf(stderr, "alert_reply: Access granted!\n");
      break;
    default:
      fprintf(stderr, "alert_reply: Bad reply type!\n");
  }
}
