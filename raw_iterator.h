#ifndef RAW_ITERATOR_H
#define RAW_ITERATOR_H


#include <stdint.h>


typedef struct {
  uint8_t* data;       // Pointer to the raw bytes
  uint8_t* curr; // Current position
  size_t size;
} raw_iterator;


void rit_init(raw_iterator* rit, uint8_t* data, size_t size);


void rit_reset(raw_iterator* rit);


void rit_read(raw_iterator* rit, size_t n, void* saved);


void rit_write(raw_iterator* rit, size_t n, void const* src);


#endif // RAW_ITERATOR_H
