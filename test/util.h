#ifndef __UTIL_H__
#define __UTIL_H__

#if defined(_MSC_VER)
  #define UNUSED
#elif defined(__GNUC__) || defined(__clang__)
  #define UNUSED __attribute__((unused))
#else
  #define UNUSED

#endif // __UTIL_H__
