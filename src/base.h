#ifndef _H_BASE_DEFS
#define _H_BASE_DEFS

// clang-format off
#include <stdbool.h>
#include <stdint.h>
// clang-format on

#define global static
#define internal static
#define local_persist static

#define PI32 3.1415926536f
#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))

// clang-format off
#define KB(val) ((val)   * 1024LL)
#define MB(val) (KB(val) * 1024LL)
#define GB(val) (MB(val) * 1024LL)
#define TB(val) (GB(val) * 1024LL)
// clang-format on

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t usize;

typedef float f32;
typedef double f64;

#endif // _H_BASE_DEFS
