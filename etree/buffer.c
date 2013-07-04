/**
 * buffer.c - single-thread buffer manager implementing LRU replacement policy
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "buffer.h"

/* various offsets for quick pointer manipulation */
static int lruln_offset, hashln_offset;

static bcb_t *findvictimbcb(buffer_t *buf);
static bcb_t *findbcb(buffer_t *buf, pagenum_t pagenum);
static uint32_t hash(uint32_t htsize, pagenum_t pagenum);
static uint32_t safebcbnum(buffer_t *buf, void *pageaddr, const char *fnname);

static int io_write(int fd, pagenum_t pageid, const void *src, size_t size);
static int io_read(void *dest, int fd, pagenum_t pageid, size_t size);

/*
 * buffer_init - create a buffer of size framecount * pagesize
 *
 * - open the file as specified by the flags 
 * - return a pointer to the buffer if OK, NULL on error;
 *
 */
buffer_t *buffer_init(const char *filename, int flags, size_t framecount, 
                      uint32_t pagesize)
{
    buffer_t *buf;
    int i;
    void *baseptr, *memberptr, *curbcbptr;

    /* initialize the file related field */
    if ((buf = (buffer_t *)malloc(sizeof(buffer_t))) == NULL) {
        /* out of memory, application should invoke perror() */
        return NULL;
    }
    if ((buf->filename = (char *)malloc(strlen(filename) + 1)) == NULL) {
        /* out of memory */
        return NULL;
    }
    strcpy(buf->filename, filename);
    if ((buf->fd = open(filename, flags & (~O_INCORE), 
                        S_IRUSR|S_IWUSR|S_IRGRP)) == -1){
        /* file open error, application should invoke perror() */
        return NULL;
    }
    buf->flags = flags;        
    
    /* initialize the buffer pool and control blocks */
    buf->pagesize = pagesize;
    buf->framecount = framecount;
    if ((buf->pool = malloc(framecount * (size_t)pagesize)) == NULL) {
        /* out of memory */
        return NULL;
    }
    if ((buf->bcbtable = (bcb_t *)malloc(framecount * sizeof(bcb_t))) == NULL){
        /* out of memory */
        return NULL;
    }

    /* initialize free bcb list and set bcb-related pointer offsets*/
    buf->freecount = framecount;
    dlink_init(&buf->freebcblist);
    
    curbcbptr = (char *)buf->pool - (size_t)pagesize;
    for (i = 0; i < framecount; i++) {
        curbcbptr = (char *)curbcbptr + (size_t)pagesize;

        buf->bcbtable[i].pagenum = -1;
        buf->bcbtable[i].pageaddr = curbcbptr;
        buf->bcbtable[i].lruln.next = buf->bcbtable[i].lruln.prev = NULL;
        /* link free list */
        dlink_insert(&buf->freebcblist, &buf->bcbtable[i].hashln); 
        buf->bcbtable[i].refcount = 0;
        buf->bcbtable[i].modified = 0; /* not fixed, not dirty */
    }

    baseptr = &buf->bcbtable[0];
    memberptr = &(buf->bcbtable[0].lruln);
    lruln_offset = (char *)memberptr - (char *)baseptr;
    memberptr = &(buf->bcbtable[0].hashln);
    hashln_offset = (char *)memberptr - (char *)baseptr;
    
    /* initialize current LRU list to contain nothing */
    dlink_init(&buf->bcblru);

    /* initialize bcb hash table */
    buf->bcbhtsize = framecount;
    if ((buf->bcbhashtable = (dlink_t *)
         malloc(buf->bcbhtsize * sizeof(dlink_t))) == NULL) {
        /* out of memory */
        return NULL;
    }
    for (i = 0; i < buf->bcbhtsize; i++) dlink_init(&buf->bcbhashtable[i]);

    buf->reqs = buf->hits = buf->hitlookups = buf->misslookups = 0;
    
    return buf;
}


/*
 * buffer_destroy - destroy the buffer
 *
 * - flush modified pages to disk if it's out-of-core
 * - release memory
 * - return 0 if OK, -1 on error
 *
 */
int buffer_destroy(buffer_t *buf)
{
    int res = 0;

    if ((buf->flags & O_RDONLY) != 0) {
        if (close(buf->fd) != 0) {
            perror("buffer_destroy: close");
        }
    }
    else if ((buf->flags & O_INCORE) != 0) {
        if (close(buf->fd) != 0) {
            perror("buffer_destroy: close");
        }
        unlink(buf->filename);
    } else { 
        dlink_t *curlink;

        curlink = buf->bcblru.next;

        while (curlink != &buf->bcblru) {
            bcb_t *curbcb;
            curbcb = (bcb_t *)((char *)curlink - lruln_offset);

            if (curbcb->refcount != 0) {
                fprintf(stderr, "buffer_destory warning (%s) : ", 
                        buf->filename);
                if (sizeof(long int) == 8) 
                    fprintf(stderr, "page %ld's refcount is %d\n",
                            (long int)curbcb->pagenum, curbcb->refcount);
                else
                    fprintf(stderr, "page %lld's refcount is %d\n",
                            (long long int)curbcb->pagenum, curbcb->refcount);
            }


            if (curbcb->modified != 0) 
                if (io_write(buf->fd, curbcb->pagenum,
                             curbcb->pageaddr, (size_t)buf->pagesize) != 0) {
                    fprintf(stderr, "buffer_destroy (%s) : io_write failed\n",
                            buf->filename);
                    res = -1;
                }
            curlink = curlink->next;
        }
        if (close(buf->fd) != 0) {
            fprintf(stderr, "buffer_destory (%s): close file fail\n",
                    buf->filename);
            perror("close");
            res = -1;
        }
    }

    /* release the hash table , bcbtable and the bufferpool*/
    free(buf->filename);
    free(buf->pool);
    free(buf->bcbtable);
    free(buf->bcbhashtable);
    free(buf);

    return res;
}


/*
 * buffer_emptyfix - allocate an empty slot for pagenum
 *
 * - LFS in RH Linux kernel 2.4 limits the size of the file to 18TB
 * - return the pointer to the page if OK, NULL on error
 *
 */
void *buffer_emptyfix(buffer_t *buf, pagenum_t pagenum)
{
    bcb_t *hitbcb;
    uint32_t hashnum;

    if (buf->freecount > 0) { 
        dlink_t *nextfree;

        nextfree = buf->freebcblist.next;
        dlink_delete(nextfree);
        buf->freecount--;
        hitbcb = (bcb_t *)((char *)nextfree - hashln_offset);
    } else {
        /* find a frame by victiming an unfixed page */
        if ((hitbcb = findvictimbcb(buf)) == NULL) {
            /* no available frame */
            return NULL;
        }
        if (hitbcb->modified == 1) {
            if (io_write(buf->fd, hitbcb->pagenum, 
                         hitbcb->pageaddr, (size_t)buf->pagesize) != 0) {
                /* io_write failed */
                return NULL;
            };      
        }
        /* remove the this to-use bcb from its hash list and the LRU list*/
        dlink_delete(&hitbcb->hashln);
        dlink_delete(&hitbcb->lruln);
    }

    hitbcb->pagenum = pagenum;
    hitbcb->modified = 0;
    hitbcb->refcount = 1;

    /* add the new page to the right hashtable entry */
    hashnum = hash(buf->bcbhtsize, pagenum);
    dlink_insert(&buf->bcbhashtable[hashnum], &hitbcb->hashln);

    /* put to the end of the LRU list*/
    dlink_insert(buf->bcblru.prev, &hitbcb->lruln); 
    
    return hitbcb->pageaddr;
}


/*
 * buffer_fix - fix the page with pagenum in the buffer pool
 *
 * - try to locate the page in buffer pool
 * - if no hit, read in the page
 * - possibly evict others
 * - LFS in RH Linux kernel 2.4 limits the size of the file to 18TB
 * - return pointer to the cached page if OK, NULL on error
 *
 */
void * buffer_fix(buffer_t *buf, pagenum_t pagenum)
{
    bcb_t *hitbcb;
    int hit = 0;  /* indicate whether there is a hit or not */

    buf->reqs++;

    if ((hitbcb = findbcb(buf, pagenum)) != NULL) { 
        /* hit in cache */
        hit = 1;
        buf->hits++;

        hitbcb->refcount++;
        dlink_delete(&hitbcb->lruln);
    }  
    else if (buf->freecount > 0) { 
        /* miss but there are free frames*/
        dlink_t *nextfree;

        nextfree = buf->freebcblist.next;
        dlink_delete(nextfree);
        buf->freecount--;
        hitbcb = (bcb_t *)((char *)nextfree - hashln_offset);
    } 
    else { 
        /* find a frame by victiming an unfixed page */
        if ((hitbcb = findvictimbcb(buf)) == NULL) {
            /* no available frame */
            return NULL;
        }
        if (hitbcb->modified == 1) {
            if (io_write(buf->fd, hitbcb->pagenum, 
                         hitbcb->pageaddr, (size_t)buf->pagesize) != 0) {
                /* io_write failed */
                return NULL;
            };      
        }
        /* remove the this to-use bcb from its hash list and LRU list*/
        dlink_delete(&hitbcb->hashln);
        dlink_delete(&hitbcb->lruln);
    }
        
    if (!hit) {  /* initialize the bcb structure */
        uint32_t hashnum;

        hitbcb->pagenum = pagenum;
        hitbcb->modified = 0;
        hitbcb->refcount = 1;
        if (io_read(hitbcb->pageaddr, buf->fd, hitbcb->pagenum, 
                    (size_t)buf->pagesize)!= 0) {
            /* io_read failed */
            
            /* return the bcb to free list */
            dlink_insert(&buf->freebcblist, &hitbcb->hashln);
            buf->freecount++;
            return NULL;
        }
        
        /* add the new page to the right hashtable entry */
        hashnum = hash(buf->bcbhtsize, pagenum);
        dlink_insert(&buf->bcbhashtable[hashnum], &hitbcb->hashln);
    }

    /* put to the end of the LRU list*/
    dlink_insert(buf->bcblru.prev, &hitbcb->lruln); 

    return hitbcb->pageaddr;
}


/*
 * buffer_ref - increment buffer pool page refcount by 1 and return 
 *              the new refcount
 *
 * safety checking and referece count checking
 *
 */
int buffer_ref(buffer_t *buf, void *pageaddr)
{
    uint32_t bcbnum = safebcbnum(buf, pageaddr, "buffer_ref");
    
    buf->bcbtable[bcbnum].refcount++;
    return buf->bcbtable[bcbnum].refcount;
}


/*
 * buffer_unref - decrement refcount by 1 and  return the new ref count  
 *
 * safety checking and referece count checking
 *  
 */
int buffer_unref(buffer_t *buf, void *pageaddr)
{
    uint32_t bcbnum = safebcbnum(buf, pageaddr, "buffer_unref");
    
    buf->bcbtable[bcbnum].refcount--;
    return buf->bcbtable[bcbnum].refcount;
}



/*
 * buffer_pagenum - return the page number correpsonding to pageaddr
 *
 * safety checking and referece count checking
 * 
 */
pagenum_t buffer_pagenum(buffer_t *buf, void *pageaddr)
{
    uint32_t bcbnum = safebcbnum(buf, pageaddr, "buffer_pagenum");
    
    return buf->bcbtable[bcbnum].pagenum;
}


/*
 * buffer_mark - mark the current page as modified
 *
 * safety checking and referece count checking 
 *
 */
void buffer_mark(buffer_t *buf, void *pageaddr)
{
    uint32_t bcbnum = safebcbnum(buf, pageaddr, "buffer_mark");

    buf->bcbtable[bcbnum].modified = 1;
    return;
}    


/*
 * buffer_isdirty - return the "modified" field of the page
 *
 * safety checking and referece count checking 
 *
 */
int buffer_isdirty(buffer_t *buf, void *pageaddr)
{
    uint32_t bcbnum = safebcbnum(buf, pageaddr, "buffer_isdirty");

    return (int)buf->bcbtable[bcbnum].modified;
}    


/*
 * hash - map a pagenum to a hash table entry 
 *
 * - division/remainder hashing function
 * - TODO: a good hash function
 * - return integer hash value
 *
 */
uint32_t hash(uint32_t htsize, pagenum_t pagenum)
{
    return (uint32_t)(pagenum % htsize);
}


/*
 * findbcb - find the buffer control block for page "pagenum"
 *
 * - search the hash list 
 * - return pointer to bcb if found , NULL if not
 *
 */
bcb_t *findbcb(buffer_t *buf, pagenum_t pagenum)
{
    uint32_t hashnum;
    dlink_t *curlink ;
    bcb_t *curbcb = NULL;
    int lookups = 0; 

    hashnum = hash(buf->bcbhtsize, pagenum);
    curlink = buf->bcbhashtable[hashnum].next;
    while (curlink != &buf->bcbhashtable[hashnum]) {
        lookups++;
        curbcb = (bcb_t *)((char *)curlink - hashln_offset);
        if (curbcb->pagenum == pagenum) {
            buf->hitlookups += lookups;
            break;
        }
        else curlink = curlink->next;
    }

    if (curlink == &buf->bcbhashtable[hashnum]) {
        /* it's a miss */
        buf->misslookups += lookups;
        curbcb = NULL;
    }
    return curbcb;
}


/*
 * findvictimbcb - find a victim page to evict
 *
 * return the pointer to the victim bcb if found, NULL on error
 *
 */
bcb_t *findvictimbcb(buffer_t *buf)
{
    dlink_t *curlink;

    curlink = buf->bcblru.next;
    while (curlink != &buf->bcblru) {
        bcb_t *curbcb;
        curbcb = (bcb_t *)((char *)curlink - lruln_offset);
        if (curbcb->refcount == 0) return curbcb;
        else curlink = curlink->next;
    }

    /* well, if reaches this point, all bcb's are being actively used */
    return NULL;
}


/*
 * io_read - read the buffer page from the filesystem
 *
 * return 0 if OK , -1 on error
 */
int io_read(void *dest, int fd, pagenum_t pageid, size_t size)
{
    pagenum_t offset = pageid * size;

    if (lseek(fd, offset, SEEK_SET) == -1) {
        /* perror("io_read() : lseek"); */
        return -1;
    }
    if (read(fd, dest, size) != size) {
        /* perror("io_read() : read");*/
        return -1;
    }
    return 0;
}   


/*
 * io_write - write the buffer page to the filesystem
 *
 */
int io_write(int fd, pagenum_t pageid, const void *src, size_t size)
{
    pagenum_t offset = pageid * size;

    if (lseek(fd, offset, SEEK_SET) == -1) {
        /* fprintf(stderr, "io_write() lseek fd of %d\n", fd);
           perror("io_write() : lseek"); */
        return -1;
    }
    if (write(fd, src, size) != size) {
        /*  perror("io_write() : write"); */
        return -1;
    }
    return 0;
}


/*
 * buffer_showusage - printout various buffer usage statistics
 *
 */
void buffer_showusage(buffer_t *buf, FILE *fp)
{
    int i, usedcount, maxlen, minlen, totallen;
    float htpercentile, avglen, avghitlen, avgmisslen, hitpercentile;

    usedcount = 0;
    maxlen = 0; minlen = 100; 
    totallen = 0;
    for (i = 0; i < buf->bcbhtsize; i++) {
        int curlen = 0;
        dlink_t *curlink = buf->bcbhashtable[i].next;

        if (curlink !=  &buf->bcbhashtable[i]) 
            usedcount++;
        else 
            continue; /* ignore empty hash entry */
        while (curlink != &buf->bcbhashtable[i]) {
            bcb_t *curbcb;

            curlen++;
            curbcb = (bcb_t *)((char *)curlink - hashln_offset);


            curlink = curlink->next;
        }
        maxlen = (maxlen > curlen) ? maxlen : curlen;
        minlen = (minlen < curlen) ? minlen : curlen;
        totallen += curlen;
    }
    htpercentile = usedcount * 100.0 / buf->bcbhtsize;
    hitpercentile = buf->hits * 100.0 / buf->reqs;

    avglen = totallen * 1.0 / usedcount;
    avghitlen = buf->hitlookups * 1.0 / buf->hits;
    avgmisslen = buf->misslookups * 1.0 / (buf->reqs  - buf->hits);

    fprintf(fp, "Bufer usage statistics:\n\n");

    fprintf(fp, "File name:\t\t\t%s\n\n", buf->filename);
    if (sizeof(long int) == 8) {
        fprintf(fp, "Requests:\t\t\t%lu\n", (unsigned long int)buf->reqs);
        fprintf(fp, "Hits:\t\t\t\t%lu\n", (unsigned long int)buf->hits);
    } else {
#ifdef ALPHA
        fprintf(fp, "Requests:\t\t\t%lu\n", (unsigned long)buf->reqs);
        fprintf(fp, "Hits:\t\t\t\t%lu\n", (unsigned long)buf->hits);
#else
        fprintf(fp, "Requests:\t\t\t%qu\n", (unsigned long long)buf->reqs);
        fprintf(fp, "Hits:\t\t\t\t%qu\n", (unsigned long long)buf->hits);
#endif
    }

    fprintf(fp, "Hit ratio:\t\t\t%.2f%%\n\n", hitpercentile);
    fprintf(fp, "Average hit lookups:\t\t%.2f\n", avghitlen);
    fprintf(fp, "Average miss lookups:\t\t%.2f\n\n", avgmisslen);
    
    fprintf(fp, "Hash table usage:\t\t%.2f%%\n", htpercentile);
    fprintf(fp, "Average hash entry length:\t%.2f\n", avglen);
    fprintf(fp, "Maximum hash entry length:\t%d\n", maxlen);
    fprintf(fp, "Minimum hash entry length:\t%d\n\n", minlen);

    return;
}

/*
 * safebcbnum - return the bcb number of pageaddr
 *
 *  - check boundary (avoid segmentation fault)
 *  - check alignment (protect other page frames)
 *  - check reference count (don't touch a page that's not "fixed")
 *  - return the bcb num if ok , exit -1 on error
 */
uint32_t safebcbnum(buffer_t *buf, void *pageaddr, const char *funcname)
{
    int64_t offset = (int64_t)((char *)pageaddr - (char *)buf->pool);
    uint32_t bcbnum = (uint32_t)(offset / buf->pagesize);

    if ((offset < 0) || (bcbnum >= buf->framecount)) {
        fprintf(stderr, "%s: pageaddr %p is out of of buffer pool.\n",
                funcname, pageaddr);
        exit(-1);
    }

    if ((int64_t)bcbnum * buf->pagesize != offset) {
        fprintf(stderr, "%s: pageaddr %p is not aligned properly.\n",
                funcname, pageaddr);
        exit(-1);
    }
    
    if (buf->bcbtable[bcbnum].refcount == 0) {
        fprintf(stderr, "%s: pageaddr %p is not allocated.\n",
                funcname, pageaddr);
        exit(-1);
    }

    return bcbnum;
}
