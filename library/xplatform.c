/*
 * xplatform.c - portability support 
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
 * contributions:
 * - Julio Lopez <jclopez@cs.cmu.edu>:  extended platform types
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xplatform.h"

/*
 * type_desc_t - type metadata, i.e., description.
 */
typedef struct _type_desc_t {
    char*	type_name;	/* type name			   */
    type_id_t	tid;		/* type id			   */
    type_id_t	canon_tid;	/* canonical type id		   */
    unsigned	alignment;	/* updated at runtime		   */
    size_t	size;		/* type size in bytes		   */
    const char*	format_spec;	/* default printf format specifier */
} schema_type_desc_t;

/*
 * _type_table - static type table for valid schema types
 */
static schema_type_desc_t _type_table[] = {
    { "",          etid_unknown, etid_unknown, 1, 0,                  "%%"   },
    { "char",      etid_char,    etid_int8,    1, sizeof (int8_t),    "%c"   },
    { "int8_t",    etid_int8,    etid_int8,    1, sizeof (int8_t),    "%c"   },
    { "uint8_t",   etid_uint8,   etid_uint8,   1, sizeof (uint8_t),   "%c"   },
    { "int16_t",   etid_int16,   etid_int16,   1, sizeof (int16_t),   "%hd"  },
    { "uint16_t",  etid_uint16,  etid_uint16,  1, sizeof (uint16_t),  "%hu"  },
    { "int32_t",   etid_int32,   etid_int32,   1, sizeof (int32_t),   "%d"   },
    { "uint32_t",  etid_uint32,  etid_uint32,  1, sizeof (uint32_t),  "%u"   },
    { "int64_t",   etid_int64,   etid_int64,   1, sizeof (int64_t),   "%ll"  },
    { "uint64_t",  etid_uint64,  etid_uint64,  1, sizeof (uint64_t),  "%ull" },
    { "float",     etid_float,   etid_float32, 1, sizeof (float),     "%hf"  },
    { "float32_t", etid_float32, etid_float32, 1, sizeof (float),     "%hf"  },
    { "double",    etid_double,  etid_float64, 1, sizeof (double),    "%g"   },
    { "float64_t", etid_float64, etid_float64, 1, sizeof (double),    "%g"   },
};


static const size_t _table_type_length =
    sizeof (_type_table) / sizeof (schema_type_desc_t);


/* 
 * testplatform - determine the current platform 
 *
 */
static platform_t xplatform_testplatform();
static void xplatform_init_table();

/*
 * xplatform_testendian - determine the endianess of the system
 *
 * - set an interger variable to 1 
 * - return little if the lower order byte is nonzero, big otherwise
 *
 */
endian_t xplatform_testendian()
{
    uint32_t i = 1;
    uint8_t *loworderbyte;

    loworderbyte = (uint8_t *)&i;
    if (*loworderbyte != 0) return little;
    else return big;
}

    
/*
 * xplatform_swapbytes - swap the bytes between two different byte ordering
 *
 * No boundary overflow checking, always return
 *
 */
void xplatform_swapbytes(void *to, const void *from, int32_t size)
{
    int32_t i;

    const uint8_t *frombyteptr = (const uint8_t *)from;
    uint8_t *tobyteptr = (uint8_t *)to + (size - 1);

    for (i = 0; i < size; i++) {
        *tobyteptr = *frombyteptr;

        frombyteptr++;
        tobyteptr--;
    }
    return;
}


/*
 * xplatform_createstruct - create a scb for a schema on the current 
 *                          platform (hardware/compiler)
 *
 * return pointer the a scb_t if OK, NULL on error
 *
 */
scb_t* xplatform_createscb (schema_t* schema)
{
    scb_t *scb;
    platform_t ptfm;
    int32_t memberind, startoffset;

    if ((scb = (scb_t *)malloc(sizeof(scb_t))) == NULL) {
        perror("xplatform_createscb: malloc scb_t");
        return NULL;
    }

    scb->membernum = schema->fieldnum;
    if ((scb->member = (member_t *)malloc(scb->membernum * sizeof(member_t)))
        == NULL) {
        perror("xplatform_createscb: malloc member array");
        free(scb);
        return NULL;
    }

    xplatform_init_table();
    ptfm = xplatform_testplatform();
    scb->endian = ptfm.endian;
    scb->schema = schema;

    startoffset = 0;
    for (memberind = 0; memberind < scb->membernum; memberind++) {
        int32_t alignment; /* current member's alignment requirement */
        int32_t size;
	type_id_t tid;

        size = schema->field[memberind].size;
        switch (size) {
        case (1): alignment = ptfm.alignment[0]; break;
        case (2): alignment = ptfm.alignment[1]; break;
        case (4): alignment = ptfm.alignment[2]; break;
        case (8): alignment = ptfm.alignment[3]; break;
        default:
            fprintf(stderr, 
                    "xplatform_createscb: unknown data type size %d\n", size);
            xplatform_destroyscb(scb);
            return NULL;
        }

        if (startoffset % alignment != 0) {
            /* force alignement */
            startoffset = ((startoffset / alignment) + 1) * alignment;
	}

	tid = xplatform_gettypeid (schema->field[memberind].type);

	scb->member[memberind].tid    = tid;
        scb->member[memberind].offset = startoffset;
        scb->member[memberind].size   = size;
        startoffset += size;
    }

    return scb;
}


/*
 * xplatform_destroyscb - release memory held by the structure control
 *                        block
 *
 */
void xplatform_destroyscb(scb_t *scb)
{
    free(scb->member);
    free(scb);
    return;
}


/* 
 * testplatform - determine the current platform 
 *
 */
platform_t xplatform_testplatform()
{
    platform_t ptfm;
    struct {int8_t a; int8_t b;} align1;
    struct {int8_t a; int16_t b;} align2;
    struct {int8_t a; int32_t b;} align4;
    struct {int8_t a; int64_t b;} align8;

    ptfm.endian = xplatform_testendian();
    ptfm.alignment[0] = (int32_t)((char *)&align1.b - (char *)&align1);
    ptfm.alignment[1] = (int32_t)((char *)&align2.b - (char *)&align2);
    ptfm.alignment[2] = (int32_t)((char *)&align4.b - (char *)&align4);
    ptfm.alignment[3] = (int32_t)((char *)&align8.b - (char *)&align8);

    return ptfm;
}

/*
 * xplatform_gettypeid - get the type id for the field with the given type
 *
 * @return the type id for the specified type name
 *	   etid_unknown if no match is found.
 */
type_id_t xplatform_gettypeid (const char* str_type)
{
    type_id_t tid = etid_unknown;
    int i;

    for (i = 0; i < sizeof (_type_table) / sizeof (schema_type_desc_t); i++) {
	if (strcmp (_type_table[i].type_name, str_type) == 0) {
	    tid = _type_table[i].tid;
	    break;
	}
    }

    return tid;
}

/*
 * intitialize static global type table: _type_table[]
 */
static void xplatform_init_table()
{
    static int table_initialized = 0;

    int i;
    platform_t ptfm;

    if (table_initialized != 0) {
	return;
    }

    table_initialized = 1;
    ptfm = xplatform_testplatform();

    /* fill in the aligment field */
    for (i = 0; i < _table_type_length; i++) {
        switch (_type_table[i].size) {
        case (1): _type_table[i].alignment = ptfm.alignment[0]; break;
        case (2): _type_table[i].alignment = ptfm.alignment[1]; break;
        case (4): _type_table[i].alignment = ptfm.alignment[2]; break;
        case (8): _type_table[i].alignment = ptfm.alignment[3]; break;
        }
    }
}


/*
 * getfield - extract value from a byte string for a given field
 *
 */
void xplatform_getfield (
	const scb_t*	scb,
	void*		dst,
	const void*	src,
	int		fieldidx,
	int		swapflag
	)
{
    const void* fieldptr;
    void*	memberptr;
    int32_t	size;

    if (fieldidx < scb->schema->fieldnum) {
        /* a particular field */
        fieldptr = (const char *)src + scb->schema->field[fieldidx].offset;
        size     = scb->schema->field[fieldidx].size;

        if (size > 1 && swapflag) {
            xplatform_swapbytes(dst, fieldptr, size);
	}

        else {
            memcpy (dst, fieldptr, size);
	}

    } else {

        /* the whole structure is needed */
        int memberind;

        for (memberind = 0; memberind < fieldidx; memberind++) {
            size      = scb->schema->field[memberind].size;
            fieldptr  = (const char*)src + scb->schema->field[memberind].offset;
            memberptr = (char *)dst + scb->member[memberind].offset;

            if (size > 1 && swapflag) {
                xplatform_swapbytes (memberptr, fieldptr, size);
	    }
            else {
                memcpy (memberptr, fieldptr, size);
	    }
        }
    }

    return;
}

/*
 * xplatform_setfield - set value to proper position in a byte string (payload)
 *			for a given field
 *
 */
void
xplatform_setfield (
	const scb_t*	scb,
	void*		dst,
	const void*	src,
	int		fieldidx,
	int		swapflag
	)
{
    const void *memberptr;
    void* fieldptr;
    int32_t size;

    if (fieldidx < scb->schema->fieldnum) {
        /* a particular field */
        fieldptr = (char *)dst + scb->schema->field[fieldidx].offset;
        size     = scb->schema->field[fieldidx].size;

        if (size > 1 && swapflag) {
            xplatform_swapbytes (fieldptr, src, size);
	} else {
            memcpy (fieldptr, src, size);
	}

    } else {
        /* the whole structure is needed */
        int memberind;

        for (memberind = 0; memberind < fieldidx; memberind++) {
            size       = scb->schema->field[memberind].size;
            fieldptr   = (char *)dst + scb->schema->field[memberind].offset;
            memberptr  = (char *)src + scb->member[memberind].offset;

            if (size > 1 && swapflag) {
                xplatform_swapbytes (fieldptr, memberptr, size);
	    } else {
                memcpy(fieldptr, memberptr, size);
	    }

        }
    }

    return;
}

/*
 * xplatform_getfieldaddr - get a pointer to the start of a field inside the
 *			    payload provided in src
 */
void* xplatform_getfieldaddr (const scb_t* scb, void* src, int fieldidx)
{
    return (fieldidx < scb->membernum)
	? (char*)src + scb->member[fieldidx].offset
	: NULL;
}

int xplatform_print_field1 (const char* fmt, void* src, FILE* stream)
{
    return fprintf (stream, fmt, *(char*)src);
}

/*
 * xplatform_print_field2 - print a 2-byte long field.  This routine performs
 *			    no byte swaping.
 *			   
 */
int xplatform_print_field2 (const char* fmt, void* src, FILE* stream)
{
    uint16_t val;

    /* is it already aligned or aligment is not needed? */
    if ((size_t)src % _type_table[etid_int16].alignment == 0) {
	val = *(uint16_t*)src;
    }

    else {
	memcpy (&val, src, 2);
    }

    return fprintf (stream, fmt, val);
}

int xplatform_print_field4 (const char* fmt, void* src, FILE* stream)
{
    uint32_t val;

    /* is it already aligned or aligment is not needed? */
    if ((size_t)src % _type_table[etid_int32].alignment == 0) {
	/* no aligment required or src already aligned */
	val = *(uint32_t*)src;
    }

    else {
	memcpy (&val, src, 4);
    }

    return fprintf (stream, fmt, val);
}

int xplatform_print_field8 (const char* fmt, void* src, FILE* stream)
{
    uint64_t val;

    if ((size_t)src % _type_table[etid_int64].alignment == 0) {
	/* no aligment required or src already aligned */
	val = *(uint64_t*)src;
    }

    else {
	memcpy (&val, src, 8);
    }

    return fprintf (stream, fmt, val);
}

void xplatform_hexstr (void* src, size_t size, char* buf, size_t buflen)
{
    static char hex_digit_table[] = { '0', '1', '2', '3', '4', '5', '6', '7',
				      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    char* ptr = src;
    size_t  i = (size < buflen/2) ? size : buflen/2;	/* min (size, buflen) */

    for (; i > 0; i--, buf +=2, ptr++) {
	buf[0] = hex_digit_table [((*ptr)>>4) & 0x0F];	/* 4 hi-order bits */
	buf[1] = hex_digit_table [(*ptr) & 0x0F];	/* 4 lo-order bits */
    }

    if (size * 2 < buflen) {
	buf[size*2] = '\0';	/* terminating '\0' if there's space */
    }

    return;
}

int xplatform_hexprint (FILE* stream, void* src, size_t size)
{
    int ret = -1;
    size_t buflen = 2*size + 1;
    /* this could be done with automatic stack allocation in gcc */
    char*  buf    = (char*)malloc (buflen * sizeof (char));

    xplatform_hexstr (src, size, buf, buflen);
    buf [buflen-1] = '\0';
    ret = fputs (buf, stream);
    free (buf);

    return (ret > 0) ? 0 : -1;
}

int xplatform_printpayload (const scb_t* scb, void* src, FILE* stream)
{
    int    ret = 0;
    int    i;
    int    len = scb->membernum;
    int    size;
    char*  field_ptr;
    const char* fmt;
    type_id_t	type;


    if (NULL == scb) {
	return -1;
    }

    for (i = 0; i < len && ret == 0; i++) {
	size       = scb->schema->field[i].size;
	field_ptr  = (char*)src + scb->member[i].offset;
	type	   = scb->member[i].tid;
	fmt	   = _type_table[type].format_spec;

	switch (size) {
	case 1: ret = xplatform_print_field1(fmt, field_ptr, stream); break;
	case 2: ret = xplatform_print_field2(fmt, field_ptr, stream); break;
	case 4: ret = xplatform_print_field4(fmt, field_ptr, stream); break;
	case 8: ret = xplatform_print_field8(fmt, field_ptr, stream); break;
	    /* add default to print hex representation of the field */
	default: ret = xplatform_hexprint (stream, field_ptr, size);
        }

	fputc (' ', stream);

	ret = (ret < 0) ? -1 : 0;
    }

    return ret;
}
