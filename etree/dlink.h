/*
 * dlink.h - auxilliary data structure for doubly linked list operations
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
#ifndef DLINK_H
#define DLINK_H

typedef struct dlink_t {
    struct dlink_t *prev;
    struct dlink_t *next;
} dlink_t;


void dlink_init(dlink_t *sentinel);
void dlink_insert(dlink_t *sentinel, dlink_t *newlink);
void dlink_delete(dlink_t *oldlink);
void dlink_addstub(dlink_t *sentinel, dlink_t *firstlink);

#endif
