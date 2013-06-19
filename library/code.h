/*
 * code.h -  conversion between different octant address and locational code
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

#ifndef CODE_H
#define CODE_H

#ifdef ALPHA
#include "etree_inttypes.h"
#else
#include <inttypes.h>
#endif


#include "etree.h"

int code_addr2key(etree_t *ep, etree_addr_t addr, void *key);
int code_key2addr(etree_t *ep, void *key, etree_addr_t *paddr);

int code_isancestorkey(const void *ancestorkey, const void *childkey);
int code_derivechildkey(const void *key, void *childkey, int branch);
int code_extractbranch(const void *morton, int level);
void code_setbranch(void *morton, int level, int branch);
void code_setlevel(void *key, int level, etree_type_t type);

int code_comparekey(const void *key1, const void *key2, int size);

void code_morton2coord(int bits, void *morton, etree_tick_t *px, 
                       etree_tick_t *py, etree_tick_t *pz);

void code_coord2morton(int bits, etree_tick_t x, etree_tick_t y, 
                       etree_tick_t z, void *morton);

#endif /* CODE_H */




