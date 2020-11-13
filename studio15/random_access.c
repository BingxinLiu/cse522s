/*
  ONE-SIDED HPCC RandomAccess benchmark

  According to the rules, there are to be 4 times as many updates as there are
  entries in the table. A random number is generated, and the lower bits are
  used as an index into the table. The value of that table entry is XORed with
  the random number and then stored.

  The next random number is generated using a shift-register and an HPCC-spec
  polynomial.

  Each table entry is a 64-bit integer.
  
  The test is supposed to be run with the largest table size at an
  even power of 2 that will fit into available memory. (Note that each
  entry is an 8-byte integer.)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>

#include <sys/time.h>

#define ZERO64B 0LL
#define POLY 0x0000000000000007ULL
#define PERIOD 1317624576693539401LL

static uint8_t 
iteration(uint64_t    * table,
          unsigned long tbl_entries,
          uint64_t      ran)
{
    uint64_t idx;
    unsigned long long access;
    int offset;


    /* 
     The HPCC spec is 4 * table size updates total
     The polynomial used in the shift register is provided by the
     HPCC.
    */
    for (idx = 0; idx < tbl_entries * 4; idx++) {
        /* mask lower bits */
        offset = ran & (tbl_entries - 1);
        table[offset] ^= ran;
        ran = (ran << 1) ^ ((int64_t) ran < ZERO64B ? POLY : ZERO64B); 
    }

    return (uint8_t)ran;
}

static int
__run_kernel(unsigned long long iterations,
             uint64_t         * array,
             unsigned long      array_entries,
             uint64_t           ran)
{
    unsigned long long iter;
    struct timeval t1, t2;
    int fd, status;
    uint8_t dummy;

    for (iter = 0; iter < iterations; iter++) {
        dummy = iteration(array, array_entries, ran);
        printf("Iteration %llu finished\n", iter);
    }

    return 0;
}

/* 
   Utility routine to start random number generator at Nth step
   (shameless lifted from public domain HPCC version)
 */
uint64_t
HPCC_starts(int64_t n)
{
  int i, j;
  uint64_t m2[64];
  uint64_t temp, ran;

  while (n < 0) n += PERIOD;
  while (n > PERIOD) n -= PERIOD;
  if (n == 0) return 0x1;

  temp = 0x1;
  for (i=0; i<64; i++) {
    m2[i] = temp;
    temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
    temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
  }

  for (i=62; i>=0; i--)
    if ((n >> i) & 1)
      break;

  ran = 0x2;
  while (i > 0) {
    temp = 0;
    for (j=0; j<64; j++)
      if ((ran >> j) & 1)
        temp ^= m2[j];
    ran = temp;
    i -= 1;
    if ((n >> i) & 1)
      ran = (ran << 1) ^ ((int64_t) ran < 0 ? POLY : 0);
  }

  return ran;
}



static int
run_kernel(unsigned long      tbl_entries,
           unsigned long long iterations)
{
    uint64_t * table;
    uint64_t   ran;
    int        ret;

    table = (uint64_t *)malloc(sizeof(uint64_t) * tbl_entries);
    if (table == NULL) {
        printf("Out of memory\n");
        return -1;
    }

	/* get initial random offset into table */
    ran = HPCC_starts(0); 
    ret = __run_kernel(iterations, table, tbl_entries, ran);

    free(table);

    return ret;

}

static void
usage(void)
{
    printf("random_access requires args:\n"
        "  <table size (MB) per proc> (must be power of 2)\n"
    );
}

int main(int argc, char ** argv)
{
    unsigned long t_size, t_entries;
    float t_size_mb;
    uint64_t * table;

    if (argc != 2) {
        usage();
        return -1;
    }

    t_size_mb = atof(argv[1]);
    t_size    = t_size_mb * (1ULL << 20);
    t_entries = t_size / sizeof(uint64_t);

    if (t_entries & (t_entries - 1)) {
        usage();
        return -1;
    }

    return run_kernel(t_entries, 4);
}
