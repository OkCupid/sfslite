// -*-c++-*-
#pragma once
#include "sfs_config.h"
#include <cassert>

#ifdef SFS_DEBUG
#define DASSERT(_X) assert(_X)
#else
#define DASSERT(_X)
#endif
