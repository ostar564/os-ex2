#include <iostream>
#include "uthreads.h"

typedef struct TCB
{
  unsigned int tid;
  sigjmp_buf env;
  unsigned int quantuns;


}TCB;