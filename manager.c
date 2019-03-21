#include <linux/init.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/pagemap.h>
#include <linux/fsnotify.h>
#include <linux/personality.h>
#include <linux/security.h>
#include <linux/ima.h>
#include <linux/syscalls.h>
#include <linux/mount.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/device_cgroup.h>
#include <linux/fs_struct.h>
#include <linux/posix_acl.h>
#include <linux/hash.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include "../ext4/ext4.h"
#include "util.h"

static struct resource_weight total_weight = {0,0};
static DEFINE_SPINLOCK( weight_update_spl);

static atomic_t total_used_inodes = {0};
static atomic_t total_used_blocks = {0};

void inc_used_inodes(struct used_resource *ur)
{
	atomic_inc(&total_used_inodes);
	atomic_inc(&(ur->inodes));
}

void dec_used_inodes(struct used_resource *ur)
{
	atomic_dec(&total_used_inodes);
	atomic_dec(&(ur->inodes));
}

void set_weight(struct resource_weight *rw,int inode,int block)
{
	rw->inode = inode;
	rw->block = block;
}

void add_total_weight(struct resource_weight * rw)
{
	spin_lock(&weight_update_spl);
	total_weight.inode += rw->inode;
	total_weight.block += rw->block;
	spin_unlock(&weight_update_spl);
}

void sub_total_weight(struct  resource_weight *rw)
{
	spin_lock(&weight_update_spl);
	total_weight.inode -= rw->inode;
	total_weight.block -= rw->block;
	spin_unlock(&weight_update_spl);	
};


int verify_create(struct super_block *sb, struct  resource_weight *rw,struct used_resource *ur)
{
	int total_free_inodes = ext4_count_free_inodes(sb);
	int container_rest_inodes = (total_free_inodes + atomic_read(&total_used_inodes)) / (total_weight.inode) *  (rw->inode)  - atomic_read(&(ur->inodes)) ;

	if(container_rest_inodes <= 0){
		printk("total free inodes:%d\n",total_free_inodes);
		printk("total used inodes:%d\n",atomic_read(&total_used_inodes));
		printk("this container total used inodes:%d\n",atomic_read(&(ur->inodes)));
		printk("total inodes weight:%d\n",total_weight.inode);
		printk("contain's inode weight:%d\n",rw->inode);
	}

	return container_rest_inodes > 0 ? 1 : 0;
}

void printInfo(struct super_block *sb, struct  resource_weight *rw)
{
	//struct ext4_super_block *es = EXT4_SB(sb)->s_es;
	//ext4_fsblk_t free_blocks;
	/*
	int free_inodes;

	free_inodes = ext4_count_free_inodes(sb);	//ialloc.c
	printk("free inodes: %u\n", free_inodes);
	*/

	printk("total inodes weight:%d\n",total_weight.inode);
	printk("contain's inode weight:%d\n",rw->inode);
}