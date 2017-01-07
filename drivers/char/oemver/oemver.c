/******************************************************************************/
/*                                                                Date:04/2014*/
/*                                PRESENTATION                                */
/*                                                                            */
/*       Copyright 2014 TCL Communication Technology Holdings Limited.        */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/* Author :  hui.wei                                                          */
/* Email  :  hui.wei@jrdcom.com                                               */
/* Role   :                                                                   */
/* Reference documents :                                                      */
/* -------------------------------------------------------------------------- */
/* Comments :                                                                 */
/* File     :                                                                 */
/* Labels   :                                                                 */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/*     Modifications on Features list / Changes Request / Problems Report     */
/* ----------|----------------------|----------------------|----------------- */
/*    date   |        Author        |         Key          |     comment      */
/* ----------|----------------------|----------------------|----------------- */
/* 04/03/2014|hui.wei               |622499                |MMI string "###23 */
/*           |                      |                      |2#" -Call Duratio */
/*           |                      |                      |n                 */
/* ----------|----------------------|----------------------|----------------- */
/* ----------|----------------------|----------------------|----------------- */
/******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <soc/qcom/smem.h>

#include "oemver.h"
#include <linux/miscdevice.h>

static int oemver_open(struct inode* inode, struct file* filp) {
    return 0;
}

static int oemver_release(struct inode* inode, struct file* filp) {
    return 0;
}

static ssize_t oemver_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) {
    struct image_version_entry * ptable = NULL;
    int len = SMEM_IMAGE_VERSION_TABLE_SIZE;
    int num = count < IMAGE_OEM_VERSION_STRING_LENGTH ? count : IMAGE_OEM_VERSION_STRING_LENGTH;

    ptable = (struct image_version_entry *)
        (smem_get_entry(SMEM_IMAGE_VERSION_TABLE, &len,0, SMEM_ANY_HOST_FLAG));

    ptable += IMAGE_INDEX_MPSS;

    if (ptable)
        memcpy(buf, ptable->image_oem_version_string,num);

    printk(KERN_INFO"%s, image_oem_version_string=%s\n", __func__, ptable->image_oem_version_string);
    return num;
}

static struct file_operations oemver_fops = {
    .owner = THIS_MODULE,
    .open = oemver_open,
    .release = oemver_release,
    .read = oemver_read,
};


static struct miscdevice oemver = {
    MISC_DYNAMIC_MINOR,
    "oemver",
    &oemver_fops
};

static int __init oemver_init(void){

    int ret;

    ret = misc_register(&oemver);
    if (ret) {
        printk(KERN_ERR "fram: can't misc_register on minor=%d\n",
            MISC_DYNAMIC_MINOR);
        return ret;
    }

    printk(KERN_INFO "oemver driver init success");
    return ret;
}


static void __exit oemver_exit(void) {

    printk(KERN_ALERT"Destroy oemver device.\n");
    misc_deregister(&oemver);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("oemver Driver");

module_init(oemver_init);
module_exit(oemver_exit);











