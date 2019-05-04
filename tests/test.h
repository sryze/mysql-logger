#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>

#define TEST(expr) \
  do { \
    if (!(expr)) { \
      printf("FALSE: " #expr " (%s:%d)\n", __FILE__, __LINE__); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

#endif /* TEST_H */
