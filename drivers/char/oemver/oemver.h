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

#ifndef _oemver_H
#define _oemver_H


#include <linux/cdev.h>

typedef enum
{
   IMAGE_INDEX_SBL = 0,
   IMAGE_INDEX_TZ = 1,
   IMAGE_INDEX_TZSECAPP = 2,
   IMAGE_INDEX_RPM = 3,
   IMAGE_INDEX_SDI = 4,
   IMAGE_INDEX_HYPERVISOR = 5,
   IMAGE_INDEX_RESERVED6 = 6,
   IMAGE_INDEX_RESERVED7 = 7,
   IMAGE_INDEX_RESERVED8 = 8,
   IMAGE_INDEX_APPSBL = 9,
   IMAGE_INDEX_APPS = 10,
   IMAGE_INDEX_MPSS = 11,
   IMAGE_INDEX_ADSP = 12,
   IMAGE_INDEX_WCNS = 13,
   IMAGE_INDEX_VENUS = 14,
   IMAGE_INDEX_RESERVED15 = 15,
   IMAGE_INDEX_RESERVED16 = 16,
   IMAGE_INDEX_RESERVED17 = 17,
   IMAGE_INDEX_RESERVED18 = 18,
   IMAGE_INDEX_RESERVED19 = 19,
   IMAGE_INDEX_RESERVED20 = 20,
   IMAGE_INDEX_RESERVED21 = 21,
   IMAGE_INDEX_RESERVED22 = 22,
   IMAGE_INDEX_RESERVED23 = 23,
   IMAGE_INDEX_RESERVED24 = 24,
   IMAGE_INDEX_RESERVED25 = 25,
   IMAGE_INDEX_RESERVED26 = 26,
   IMAGE_INDEX_RESERVED27 = 27,
   IMAGE_INDEX_RESERVED28 = 28,
   IMAGE_INDEX_RESERVED29 = 29,
   IMAGE_INDEX_RESERVED30 = 30,
   IMAGE_INDEX_RESERVED31 = 31,
   IMAGE_INDEX_MAX = 32,
   IMAGE_INDEX_ENUM_SIZE = 99 // facilitate compiler optimization

} image_index_type;

#define SMEM_IMAGE_VERSION_TABLE_SIZE       4096
#define SMEM_IMAGE_VERSION_ENTRY_SIZE       128

#define IMAGE_INDEX_LENGTH                2
#define IMAGE_SEP1_LENGTH                 1
#define IMAGE_QC_VERSION_STRING_LENGTH      72
#define IMAGE_VARIANT_STRING_LENGTH       20
#define IMAGE_SEP2_LENGTH                 1
#define IMAGE_OEM_VERSION_STRING_LENGTH   32

struct image_version_entry
{
   char image_index[IMAGE_INDEX_LENGTH];
   char image_colon_sep1[IMAGE_SEP1_LENGTH];
   char image_qc_version_string[IMAGE_QC_VERSION_STRING_LENGTH];
   char image_variant_string[IMAGE_VARIANT_STRING_LENGTH];
   char image_colon_sep2[IMAGE_SEP2_LENGTH];
   char image_oem_version_string[IMAGE_OEM_VERSION_STRING_LENGTH];
};

#endif
