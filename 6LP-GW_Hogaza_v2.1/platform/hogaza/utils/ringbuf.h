/**
 * \file		ringbuf.h
 *
 * \brief		Ring-Buffer library definitions and declarations.
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#ifndef _RINGBUF_H
#define _RINGBUF_H

#include "contiki.h"
/*----------------------------------------------------------------------------*/

/**
 * Ring-buffer data-type definition
 */
/*----------------------------------------------------------------------------*/
typedef struct {
    volatile u8_t *pData;
    volatile u16_t nBytes;
    volatile u16_t iHead;
    volatile u16_t iTail;
    volatile u16_t len;
} ringbuf_t;
/*----------------------------------------------------------------------------*/

/**
 * EXTERNAL FUNCTIONS
 */
/*----------------------------------------------------------------------------*/
void  ringbuf_init(ringbuf_t *pBuf, u8_t *buffer, u16_t len);
u16_t ringbuf_put(ringbuf_t *pBuf, const u8_t *pData, u16_t nBytes);
u16_t ringbuf_get(ringbuf_t *pBuf, u8_t *pData, u16_t n);
u16_t ringbuf_peek(ringbuf_t *pBuf, u8_t *pData, u16_t nBytes);
u16_t ringbuf_length(ringbuf_t *pBuf);
void ringbuf_flush(ringbuf_t *pBuf);
/*----------------------------------------------------------------------------*/

#endif /*_RINGBUF_H */

/*----------------------------------------------------------------------------*/
