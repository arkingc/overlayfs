#ifndef __UTIL_H__

#define __UTIL_H__

#include <linux/types.h>

struct resource_weight{
	int inode;
	int block;
};

struct used_resource{
	atomic_t inodes;
	atomic_t blocks;
};

#endif