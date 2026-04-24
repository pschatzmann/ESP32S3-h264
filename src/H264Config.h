#pragma once

#include "RAMAllocator.h"
#include "PSRAMAllocator.h"

#ifndef DEFAULT_ALLOCATOR
#define DEFAULT_ALLOCATOR RAMAllocator<uint8_t>
#endif