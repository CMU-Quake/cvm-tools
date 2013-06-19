/*
 * btree.h - Btree with bulk (limited) insert and update support
 *
 * Copyright (c) 2003 Tiankai Tu  
 * All rights reserved.  May not be used, modified, or copied 
 * without permission.
 *
 * Tiankai Tu
 * Computer Science Department
 * Carnegie Mellon University
 * 5000 Forbes Avenue
 * Pittsburgh, PA 15213
 * tutk@cs.cmu.edu
 *
 */

#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>

#ifdef ALPHA
#include "etree_inttypes.h"
#else
#include <inttypes.h>
#endif

#ifndef PAGENUM_T
typedef off_t pagenum_t;
#define PAGENUM_T
#endif


/*
 * btree_t - runtime btree handler; a stub handler to the underlying
 *           btree control. Don't do anything with the hander!
 *
 */
typedef struct btree_t {} btree_t;


/*
 * admin routines
 *
 */
int btree_leafcapacity(btree_t *bp);
pagenum_t btree_numofpages(btree_t *bp);
off_t btree_getendoffset(btree_t *bp);
int btree_isempty(btree_t *bp);
int btree_getvaluesize(btree_t *bp);


/*
 * Initialization and cleanup routines: an application needs to 
 * provide to the btree a comparison function to determine the 
 * ordering of two keys
 *
 */

typedef int btree_compare_t(const void *key1, const void *key2, int size);
btree_t *btree_open(const char *pathname, int flags, uint32_t ksize, 
                    const char *ktype, uint32_t vsize, uint32_t pagesize, 
                    int32_t bufsize, btree_compare_t *compare, 
                    off_t startoffset);

int btree_registerschema(btree_t *bp, const char *defstring);
int btree_printschema(btree_t *bp, FILE *fp);
char *btree_getschema(btree_t *bp);
int btree_close(btree_t *bp);


/*
 * standard btree routines: stateless operations
 *
 */
int btree_insert(btree_t *bp, const void *key, const void *value, int *insed);
int btree_search(btree_t *bp, const void  *key, void *hitkey, 
                 const char *fieldname, void *value);
int btree_update(btree_t *bp, const void *key, const void *value);
int btree_delete(btree_t *bp, const void *key);



/*
 * traverse the btree leaf nodes using a cursor 
 * (in traversal cursor state)
 *
 */
int btree_initcursor(btree_t *bp, const void *key);
int btree_stopcursor(btree_t *bp);
int btree_getcursor(btree_t *bp, void *key, const char *fieldname, 
                    void *value);
int btree_advcursor(btree_t *bp);


/*
 * append to the end of the btree
 * (append cursor state)
 *
 */
int btree_beginappend(btree_t *bp, double fillratio);
int btree_append(btree_t *bp, const void *key, const void *value);
int btree_endappend(btree_t *bp);


/*
 * output usage statistics to a file
 *
 */
int btree_printstat(btree_t *bp, FILE *fp);


/*
 * support for bulk operations 
 *
 */
int btree_bulkupdate(btree_t *bp, const void *anchorkey, int count, 
                     const void *keys[], const void *values[]);
int btree_bulkinsert(btree_t *bp, int count, const void *keys[], 
                     const void *values[]);

int btree_printpayload (btree_t *bp, void* payload, FILE* stream);

/*
 * btree_printpayload - print a string representation of a record payload
 *			into the given file.
 *
 */
int btree_printpayload (btree_t *bp, void* payload, FILE* stream);

#endif /* BTREE_H */
