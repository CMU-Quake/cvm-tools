/*
 * dlink.c - implementation of doubly linked list
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

#include <stdio.h>
#include "dlink.h"


/*
 * dlink_init - initialize a doubly linked list
 *
 */
void dlink_init(dlink_t *sentinel)
{
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    return;
}


/*
 * dlink_insert - insert a new entry at the head of the d-list
 *
 */
void dlink_insert(dlink_t *sentinel, dlink_t *newlink)
{
    /* link in the new comer */
    newlink->next = sentinel->next;
    newlink->prev = sentinel;
    
    /* modify neighbors' pointers*/
    sentinel->next->prev = newlink; 
    sentinel->next = newlink;
    return;
}


/*
 * dlink_delete - delete an entry pointed by oldlink
 *
 */
void dlink_delete(dlink_t *oldlink)
{
    oldlink->prev->next = oldlink->next;
    oldlink->next->prev = oldlink->prev;
    oldlink->next = oldlink->prev = NULL;
    return;
}


/*
 * dlink_addstub - create a stub for the dlink 
 *
 */
void dlink_addstub(dlink_t *sentinel, dlink_t *firstlink)
{
    sentinel->next = firstlink;
    sentinel->prev = firstlink->prev;

    sentinel->prev->next = sentinel;
    sentinel->next->prev = sentinel;
    return;
}
