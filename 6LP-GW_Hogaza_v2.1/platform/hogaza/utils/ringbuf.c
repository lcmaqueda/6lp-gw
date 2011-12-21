/**
 * \file		ringbuf.h
 *
 * \brief		Ring-Buffer library implementation
 *
 * \author		Luis Maqueda <luis@sen.se>
 */

#include "utils/ringbuf.h"
/*---------------------------------------------------------------------------*/

/**
* @fn      ringbuf_init
*
* @brief   Initialize a ring-buffer. The buffer itself must be allocated by the
*          application.
*
* @param   pBuf - pointer to the ring-buffer
* 		   buffer - the actual buffer where data is to be stored
* 		   len	- buffer length
*
* @return  none
*/
/*----------------------------------------------------------------------------*/
void
ringbuf_init(ringbuf_t *pBuf, u8_t *buffer, u16_t len)
{        
  pBuf->nBytes = 0;
  pBuf->iHead = 0;
  pBuf->iTail = 0;
  pBuf->pData = buffer;
  pBuf->len = len;
}
/*----------------------------------------------------------------------------*/

/**
 * @fn      ringbuf_put
 *
 * @brief   Add bytes to the buffer.
 *
 * @param   pBuf - pointer to the ringbuffer
 *          pData - pointer to data to be appended to the buffer
 *          nBytes - number of bytes
 *
 * @return  Number of bytes copied to the buffer
 */
/*----------------------------------------------------------------------------*/
u16_t
ringbuf_put(ringbuf_t *pBuf, const u8_t *pData, u16_t nBytes)
{
  u16_t i;
    
  if (pBuf->nBytes + nBytes < pBuf->len) {
	i = 0;
	while(i < nBytes) {
	  pBuf->pData[pBuf->iTail] = pData[i];
	  pBuf->iTail++;
	  if (pBuf->iTail == pBuf->len) {
	  	pBuf->iTail= 0;
	  }
	  i++;
	}
	pBuf->nBytes += i;
  } else {
  	i = 0;
  }
  return i;
}
/*----------------------------------------------------------------------------*/

/**
 * @fn      ringbuf_get
 *
 * @brief   Extract bytes from the buffer.
 *
 * @param   pBuf   - pointer to the ringbuffer
 *          pData  - pointer to data to be extracted
 *          nBytes - number of bytes
 *
 * @return  Bytes actually returned
 */
/*----------------------------------------------------------------------------*/
u16_t
ringbuf_get(ringbuf_t *pBuf, u8_t *pData, u16_t nBytes)
{
  u16_t i;
    
  i = 0;
  while(i < nBytes && i < pBuf->nBytes) {
    pData[i] = pBuf->pData[pBuf->iHead];
    pBuf->iHead++;
    if (pBuf->iHead == pBuf->len) {
      pBuf->iHead = 0;
    }
    i++;
  }

  pBuf->nBytes-= i;
  return i;
}
/*----------------------------------------------------------------------------*/

/**
 * @fn      ringbuf_peek
 *
 * @brief   Read bytes from the buffer but leave them in the queue.
 *
 * @param   pBuf   - pointer to the ringbuffer
 *          pData  - pointer to data to be extracted
 *          nBytes - number of bytes
 *
 * @return  Bytes actually returned
 */
/*----------------------------------------------------------------------------*/
u16_t
ringbuf_peek(ringbuf_t *pBuf, u8_t *pData, u16_t nBytes)
{
  u16_t i,j;

  i = 0;
  j = pBuf->iHead;
  while(i < nBytes && i < pBuf->nBytes) {
    pData[i]= pBuf->pData[j];
    j++;
    if (j == pBuf->len) {
      j = 0;
    }
    i++;
  }

  return i;
}
/*----------------------------------------------------------------------------*/

/**
 * @fn      ringbuf_length
 *
 * @brief   Return the ring buffer length in bytes.
 *
 * @param   pBuf- pointer to the buffer
 *
 * @return  Number of bytes present.
 */
/*----------------------------------------------------------------------------*/
u16_t
ringbuf_length(ringbuf_t *pBuf)
{
  return pBuf->nBytes;
}
/*----------------------------------------------------------------------------*/

/**
 * @fn      ringbuf_flush
 *
 * @brief   Flush the buffer.
 *
 * @param   pBuf- pointer to the buffer
 */
/*----------------------------------------------------------------------------*/
void
ringbuf_flush(ringbuf_t *pBuf)
{
  ringbuf_init(pBuf, pBuf->pData, pBuf->len);
}
/*----------------------------------------------------------------------------*/
