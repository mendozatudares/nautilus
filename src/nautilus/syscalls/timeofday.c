/// At a file level, the utility functions here should probably be moved
/// elsewhere in Nautilus, and the syscalls should be split into their own files
/// for consistency

#include <nautilus/nautilus.h>

#define NSEC_PER_SEC (uint64_t)1000000000
#define NSEC_PER_USEC (uint64_t)1000U

/// Offset applied when calculating the time from the cycle count
static uint64_t time_offset = 0;

/// TODO: move this struct definition somewhere useful and standard
struct timeval {
  int tv_sec;  // seconds
  int tv_usec; // microseconds
};

/// TODO: add multiarch support if rdtsc is not a compiler intrinsic
/// @return The cycle count
uint64_t get_cycles() { return rdtsc(); }

/// TODO: make this based on the arch instead of being fake
/// @return Elapsed seconds since the cycle count was reset
uint64_t cycles2ns(uint64_t cycles) {
  return cycles;
}

/// @return The time in nanoseconds
uint64_t get_time() { return cycles2ns(get_cycles()) + time_offset; }

/// @param ns The number of nanoseconds to offset against the 0 cycle count time
void set_time(uint64_t ns) { time_offset = ns - cycles2ns(get_cycles()); }

/// @param timeval_ptr struct timeval to mutate
/// @param timezone_ptr obsolete, not currently supported in Nautilus
int sys_gettimeofday(int timeval_ptr, int timezone_ptr) {
  struct timeval* tv = (struct timeval*)timeval_ptr;
  if (tv != NULL) {
    uint64_t now = get_time(); /* nanoseconds */

    tv->tv_sec = now / NSEC_PER_SEC;
    tv->tv_usec = (now % NSEC_PER_SEC) / NSEC_PER_USEC;
  }
  /// TODO: add errno support?
  return 0;
}

/// @param timeval_ptr struct timeval to mutate
/// @param timezone_ptr obsolete, not currently supported in Nautilus
int sys_settimeofday(int timeval_ptr, int timezone_ptr) {
  /// TODO: add input validation and (maybe?) errno support
  /// (in Linux, tv_usec should not be greater than 999,999)
  struct timeval* tv = (struct timeval*)timeval_ptr;
  if (tv != NULL) {
    set_time((tv->tv_sec * NSEC_PER_SEC) + (tv->tv_usec * NSEC_PER_USEC));
  }
  return 0;
}

#undef NSEC_PER_SEC
#undef NSEC_PER_USEC
