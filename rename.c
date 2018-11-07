#include <linux/init.h>
#include <linux/export.h>
#include <linux/kernel.h>
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

/**
 *  d_ancestor - search for an ancestor
 *  @p1: ancestor dentry
 *  @p2: child dentry
 *  
 *  Returns the ancestor dentry of p2 which is a child of p1, if p1 is
 *  an ancestor of p2, else NULL.
 */
struct dentry *d_ancestor(struct dentry *p1, struct dentry *p2)
{
    struct dentry *p;

    for (p = p2; !IS_ROOT(p); p = p->d_parent) {
        if (p->d_parent == p1)
            return p;
    }
    return NULL;
}

/*
 *  p1 and p2 should be directories on the same fs.
 */
struct dentry *ovl_lockfree_rename(struct dentry *p1, struct dentry *p2)
{
    struct dentry *p;

    if (p1 == p2) {
        mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
        return NULL;
    }

    p = d_ancestor(p2, p1);
    if (p) {
        mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_PARENT);
        mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_CHILD);
        return p;
    }

    p = d_ancestor(p1, p2);
    if (p) {
        mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
        mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_CHILD);
        return p;
    }

    mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
    mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_PARENT2);
    return NULL;
}

/*
 *  p1 and p2 should be directories on the same fs.
 */
struct dentry *ovl_lock_rename(struct dentry *p1, struct dentry *p2, struct mutex *m)
{
    struct dentry *p;

    if (p1 == p2) {
        mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
        return NULL;
    }

    mutex_lock(m);
    //mutex_lock(&p1->d_inode->i_sb->s_vfs_rename_mutex);

    p = d_ancestor(p2, p1);
    if (p) {
        mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_PARENT);
        mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_CHILD);
        return p;
    }

    p = d_ancestor(p1, p2);
    if (p) {
        mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
        mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_CHILD);
        return p;
    }

    mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
    mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_PARENT2);
    return NULL;
}

void ovl_unlock_rename(struct dentry *p1, struct dentry *p2, struct mutex *m)
{
    mutex_unlock(&p1->d_inode->i_mutex);
    if (p1 != p2) {
        mutex_unlock(&p2->d_inode->i_mutex);
        mutex_unlock(m);
        //mutex_unlock(&p1->d_inode->i_sb->s_vfs_rename_mutex);
    }
}

void ovl_unlock2_rename(struct dentry *p1, struct dentry *p2)
{
    mutex_unlock(&p1->d_inode->i_mutex);
    if (p1 != p2) {
        mutex_unlock(&p2->d_inode->i_mutex);
        //mutex_unlock(&p1->d_inode->i_sb->s_vfs_rename_mutex);
    }
}
