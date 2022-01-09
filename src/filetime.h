#ifndef FILETIME_H_
#define FILETIME_H_

#ifndef _WIN32
#include <cstdint>
using DWORD = uint32_t;
typedef struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;
#endif

#endif
