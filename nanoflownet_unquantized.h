#ifndef __nanoflownet_unquantized_H__
#define __nanoflownet_unquantized_H__

#define __PREFIX(x) nanoflownet_unquantized ## x

// Include basic GAP builtins defined in the Autotiler
#include "Gap.h"

#ifdef __EMUL__
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <string.h>
#endif

extern AT_HYPERFLASH_FS_EXT_ADDR_TYPE nanoflownet_unquantized_L3_Flash;
#endif