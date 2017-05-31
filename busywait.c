#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "busywait.h"

// FIXME
#include "closure.h"
#include "packet.h"


bool busy_wait_until(struct timeval timeout, try_fn fn, void* args, void* retval) {
  struct timeval curr_time; // For keeping track of timeout
  struct timeval start_time;
  struct timeval diff; // For calculating elapsed time

  gettimeofday(&start_time, NULL);

  while (!fn(args, retval)) {
    // Calculate elapsed time
    gettimeofday(&curr_time, NULL);
    timersub(&curr_time, &start_time, &diff);
    if (timercmp(&diff, &timeout, >)) {
      // Operation timed out
      return false;
    }
  }

  struct STRUCT_ARGS_NAME(recv)* sargs = (struct STRUCT_ARGS_NAME(recv)*)args;
  uint32_t* buf = (uint32_t*)sargs->buf;


  fprintf(stderr, "recvd: %0x\n", *buf);
  fprintf(stderr, "recvd: %0x\n", *(buf + 1));
  fprintf(stderr, "recvd: %0x\n", *(buf + 2));
  fprintf(stderr, "recvd: %0x\n", *(buf + 3));
  fprintf(stderr, "recvd: %0x\n", *(buf + 4));


  return true;
}


