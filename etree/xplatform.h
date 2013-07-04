/*
 * xplatform.h - portability support for various platforms
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

#ifndef XPLATFORM_H
#define XPLATFORM_H

#ifdef ALPHA
#include "etree_inttypes.h"
#else
#include <inttypes.h>
#endif 

#include <stdio.h>

#include "schema.h"

/*
 * endian_t - the endianness of the system 
 *
 */
#ifndef ENDIAN_T
#define ENDIAN_T
typedef enum endian_t { unknown_endianness = -1, little, big} endian_t;
#endif

/*
 * schema_type_id_t schema type identifiers
 *
 */
typedef enum type_id_t {
    etid_unknown = 0,
    etid_char,
    etid_int8,
    etid_uint8,
    etid_int16,
    etid_uint16,
    etid_int32,
    etid_uint32,
    etid_int64,
    etid_uint64,
    etid_float,
    etid_float32,
    etid_double,
    etid_float64
} type_id_t;


/*
 * member_t - describes the member size and alignment of a structure 
 *            corresponding to a schema on a particular platform
 *
 * name of the member is the same as that in a schema, thus omitted here
 */
typedef struct member_t {
    int32_t   offset;           /* offset in the structure               */
    int32_t   size;             /* size of the member = field_t.size     */
    type_id_t tid;		/* member type id			 */
} member_t;
    

/*
 * scb_t - "structure control block"; 
 *          records the layout of a structure corresponding to a schema
 *          for a particular platform
 *
 */
typedef struct scb_t {
    schema_t* schema;
    endian_t  endian;
    int32_t   membernum;
    member_t* member;
} scb_t;


/*
 * platform_t - a platform records the specification of the endianness of
 *              the hardware, and the structure alignment requirement for
 *              the hardware/compiler configuration.
 *
 * the alignement array contains the alignment requirement for 1-, 2-, 4-, 8-
 * byte 
 */
typedef struct platform_t {
    endian_t endian;
    int32_t  alignment[4]; 
} platform_t;



/*
 * xplatform_testendian - determine the endianness of the system 
 *
 */
endian_t xplatform_testendian();


/*
 * xplatform_swapbytes - convert between different endian format 
 *
 */
void xplatform_swapbytes(void *to, const void *from, int32_t size);


/*
 * xplatform_createstruct - create a structure instance for a schema
 *                       for a given schema
 *
 */
scb_t *xplatform_createscb(schema_t* schema);


/* 
 * xplatform_destroystruct - release memory held by the structure control
 *                           block
 *
 */
void xplatform_destroyscb(scb_t *scb);

/*
 * xplatform_gettypeid - get the type id for the field with the given type
 *
 * @return the type id for the specified type name
 *	   etid_unknown if no match is found.
 */
type_id_t xplatform_gettypeid (const char* str_type);

/*
 * getfield - extract value from a byte string for a given field
 *
 */
void xplatform_getfield (const scb_t* scb, void* dst, const void* src,
			 int fieldidx, int swapflag);


/*
 * xplatform_setfield - set value to proper position in a byte string (payload)
 *			for a given field
 *
 */
void xplatform_setfield (const scb_t* scb, void* dst, const void* src,
			 int fieldidx, int swapflag);

/*
 * xplatform_hexprint - print a hex representation of a byte stream into
 *			the specified file
 *
 */
int xplatform_hexprint (FILE* stream, void* src, size_t size);

/*
 * xplatform_printpayload - print a string representation of a record payload
 *			according to the description in the scb
 *
 */
int xplatform_printpayload(const scb_t* scb, void* src, FILE* stream);

#endif /* XPLATFORM_H */
