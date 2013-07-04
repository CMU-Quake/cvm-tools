/*
 * schema_ex.c - etree schema utility functions
 *
 * Copyright (c) 2005 Julio Lopez, 
 * All rights reserved.  May not be used, modified, or copied 
 * without permission.
 *
 * Julio Lopez
 * Department of Electrical and Computer Engineering
 * Carnegie Mellon University
 * 5000 Forbes Avenue
 * Pittsburgh, PA 15213
 * jclopez@andrew.cmu.edu
 *
 */
#include <stdio.h>
#include "etree.h"


int schemax_printpayload(etree_t *ep, void* payload, FILE* stream)
{
    /* a little hacky ... */
    return btree_printpayload (ep->bp, payload, stream);
}
