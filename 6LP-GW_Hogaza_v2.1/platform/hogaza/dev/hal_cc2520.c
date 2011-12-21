/***********************************************************************************

  Filename:     hal_cc2520.c

  Description:  This file implements the CC2520 Radio HAL.

***********************************************************************************/

/***********************************************************************************
* INCLUDES
*/

#include "hal_cc2520.h"

/***********************************************************************************
* CONSTANTS AND DEFINES
*/
#define BLOCK_SIZE  255


/***********************************************************************************
* DATATYPES
*/
typedef union {
    u16_t w;
    struct {
        u8_t b0;
        u8_t b1;
    } b;
} eword_t;


typedef union {
    u32_t dw;
    struct {
        u8_t b0;
        u8_t b1;
        u8_t b2;
        u8_t b3;
    } b;
} edword_t;

/***********************************************************************************
* LOCAL FUNCTIONS
*/
static u8_t GotException(u8_t index);
static u8_t waitException(u8_t exc, u16_t timeOut);
static void clearException(u32_t dwMap);
static void CC2520_INS_RD_ARRAY(u16_t count, u8_t  *pData);
static u8_t CC2520_INS_MEMCP_COMMON(u8_t instr, u8_t pri, u16_t count, \
    u16_t src, u16_t dest);

static u8_t CC2520_SPI_TXRX(u8_t x)
{
    CC2520_SPI_RX_NOT_READY(); 
    CC2520_SPI_TX_REG = x;
    while (!CC2520_SPI_RX_IS_READY());
    return CC2520_SPI_RX();
}

/***********************************************************************************
* @fn      GotException
*
* @brief   Check if exception is set
*
* @param  u8_t index
*
* @return  u8_t
*/
static u8_t GotException(u8_t index)
{
    u8_t exceptions = CC2520_REGRD8(CC2520_EXCFLAG0 + (index >> 3));
    if (exceptions & BV(index & 0x07)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


/***********************************************************************************
* @fn      waitException
*
* @brief   Wait for exception
*
* @param  u8_t exc
*         u16_t timeOut
*
* @return  u8_t
*/
static u8_t waitException(u8_t exc, u16_t timeOut)
{
    while (--timeOut>0) {
        if (GotException(exc))
            break;
        halMcuWaitUs(10);
    }
    return timeOut>0;
}


/***********************************************************************************
* @fn      clearException
*
* @brief   Clear exception flag
*
* @param  u32_t dwMap
*
* @return  none
*/
static void clearException(u32_t dwMap)
{
    CC2520_REGWR24(CC2520_EXCFLAG0, ~dwMap);
}


/***********************************************************************************
* @fn      CC2520_INS_RD_ARRAY
*
* @brief   Read array from CC2520
*
* @param   u16_t count -
*          u8_t  *pData -
*
* @return  none
*/
static void CC2520_INS_RD_ARRAY(u16_t count, u8_t  *pData)
{
    while (count--) {
        CC2520_SPI_RX_NOT_READY(); 
        CC2520_SPI_TX_REG = 0x00;
        while (!CC2520_SPI_RX_IS_READY());
        *pData = CC2520_SPI_RX();
        pData++;
    }
}



/***********************************************************************************
* @fn      CC2520_INS_MEMCP_COMMON
*
* @brief   memory copy
*
* @param   u8_t instr -
*          u8_t pri -
*          u16_t count -
*          u16_t src -
*          u16_t dest -
*
* @return  u8_t - status byte
*/
static u8_t CC2520_INS_MEMCP_COMMON(u8_t instr, u8_t pri, u16_t count, \
    u16_t src, u16_t dest)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(instr | pri);
    CC2520_SPI_TXRX(count);
    CC2520_SPI_TXRX((HI_UINT16(src) << 4) | HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* GLOBAL FUNCTIONS
*/

/***********************************************************************************
* @fn      CC2520_INS_WR_ARRAY
*
* @brief   Write array to CC2520
*
* @param   u16_t count -
*          u8_t  *pData -
*
* @return  none
*/
void CC2520_INS_WR_ARRAY(u16_t count, u8_t  *pData)
{
    while (count--) {
        CC2520_SPI_RX_NOT_READY(); 
        CC2520_SPI_TX_REG = *pData;
        pData++;
        while (!CC2520_SPI_RX_IS_READY());
    }
}


/***********************************************************************************
* @fn      CC2520_INS_STROBE
*
* @brief   send strobe commane
*
* @param   u8_t strobe - strobe command
*
* @return  u8_t - status byte
*/
u8_t CC2520_INS_STROBE(u8_t strobe)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(strobe);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_SNOP
*
* @brief   SNOP instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SNOP(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SNOP);
}


/***********************************************************************************
* @fn      CC2520_SIBUFEX
*
* @brief   SIBUFEX instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SIBUFEX(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SIBUFEX);
}


/***********************************************************************************
* @fn      CC2520_SSAMPLECCA
*
* @brief   Sample CCA instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SSAMPLECCA(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SSAMPLECCA);
}


/***********************************************************************************
* @fn      CC2520_SXOSCON
*
* @brief   SXOSCON instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SXOSCON(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SXOSCON);
}


/***********************************************************************************
* @fn      CC2520_STXCAL
*
* @brief   STXCAL instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_STXCAL(void)
{
    return CC2520_INS_STROBE(CC2520_INS_STXCAL);
}


/***********************************************************************************
* @fn      CC2520_SRXON
*
* @brief   SRXON instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SRXON(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SRXON);
}


/***********************************************************************************
* @fn      CC2520_STXON
*
* @brief   STXON instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_STXON(void)
{
    return CC2520_INS_STROBE(CC2520_INS_STXON);
}


/***********************************************************************************
* @fn      CC2520_STXONCCA
*
* @brief   STXONCCA instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_STXONCCA(void)
{
    return CC2520_INS_STROBE(CC2520_INS_STXONCCA);
}


/***********************************************************************************
* @fn      CC2520_SRFOFF
*
* @brief   SRFOFF instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SRFOFF(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SRFOFF);
}


/***********************************************************************************
* @fn      CC2520_SXOSCOFF
*
* @brief   SXOSCOFF instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SXOSCOFF(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SXOSCOFF);
}


/***********************************************************************************
* @fn      CC2520_SFLUSHRX
*
* @brief   SFLUSHRX instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SFLUSHRX(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SFLUSHRX);
}


/***********************************************************************************
* @fn      CC2520_SFLUSHTX
*
* @brief   SFLUSHTX instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SFLUSHTX(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SFLUSHTX);
}


/***********************************************************************************
* @fn      CC2520_SACK
*
* @brief   SACK instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SACK(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SACK);
}


/***********************************************************************************
* @fn      CC2520_SACKPEND
*
* @brief   SACKPEND instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SACKPEND(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SACKPEND);
}


/***********************************************************************************
* @fn      CC2520_SNACK
*
* @brief   SNACK instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SNACK(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SNACK);
}


/***********************************************************************************
* @fn      CC2520_SRXMASKBITSET
*
* @brief   SRXMASKBITSET instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SRXMASKBITSET(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SRXMASKBITSET);
}


/***********************************************************************************
* @fn      CC2520_SRXMASKBITCLR
*
* @brief   SRXMASKBITCLR instruction strobe
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SRXMASKBITCLR(void)
{
    return CC2520_INS_STROBE(CC2520_INS_SRXMASKBITCLR);
}


/***********************************************************************************
* @fn      CC2520_IBUFLD
*
* @brief   IBUFLD - load instruction to instruction buffer
*
* @param   u8_t i -
*
* @return  u8_t - status byte
*/
u8_t CC2520_IBUFLD(u8_t i)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_IBUFLD);
    CC2520_SPI_TXRX(i);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_SRES
*
* @brief   SRES - Reset device except SPI interface
*
* @param   none
*
* @return  u8_t - status byte
*/
u8_t CC2520_SRES(void)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_SRES);
    CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_MEMRD
*
* @brief   Read memory
*
* @param   u16_t addr
*          u16_t count
*          u8_t  *pData
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMRD(u16_t addr, u16_t count, u8_t  *pData)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_MEMRD | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_INS_RD_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_MEMRD8
*
* @brief   Read memory 8 bits
*
* @param   u16_t addr
*
* @return  u8_t - result
*/
u8_t CC2520_MEMRD8(u16_t addr)
{
    u8_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_MEMRD | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    value = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value;
}


/***********************************************************************************
* @fn      CC2520_MEMRD16
*
* @brief   Read memory 16 bits
*
* @param   u16_t addr
*
* @return  u16_t - result
*/
u16_t CC2520_MEMRD16(u16_t addr)
{
    eword_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_MEMRD | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    value.b.b0 = CC2520_SPI_TXRX(0x00);
    value.b.b1 = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value.w;
}


/***********************************************************************************
* @fn      CC2520_MEMRD24
*
* @brief   Read memory 24 bits
*
* @param   u16_t addr
*
* @return  u32_t - result
*/
u32_t CC2520_MEMRD24(u16_t addr)
{
    edword_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_MEMRD | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    value.b.b0 = CC2520_SPI_TXRX(0x00);
    value.b.b1 = CC2520_SPI_TXRX(0x00);
    value.b.b2 = CC2520_SPI_TXRX(0x00);
    value.b.b3 = 0x00;
    CC2520_SPI_END();
    return value.dw;
}


/***********************************************************************************
* @fn      CC2520_MEMWR
*
* @brief   Write memory
*
* @param   u16_t addr
*          u16_t count
*          u8_t  *pData
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMWR(u16_t addr, u16_t count, u8_t  *pData)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_MEMWR | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_INS_WR_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_MEMWR8
*
* @brief   Write memory 8 bits
*
* @param   u16_t addr
*          u8_t value
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMWR8(u16_t addr, u8_t value)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_MEMWR | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_TXRX(value);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_MEMWR16
*
* @brief   Write memory 16 bits
*
* @param   u16_t addr
*          u16_t value
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMWR16(u16_t addr, u16_t value)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_MEMWR | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(value));
    CC2520_SPI_TXRX(HI_UINT16(value));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_MEMWR24
*
* @brief   Write memory 24 bits
*
* @param   u16_t addr
*          u32_t value
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMWR24(u16_t addr, u32_t value)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_MEMWR | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(LO_UINT32(value)));
    CC2520_SPI_TXRX(HI_UINT16(LO_UINT32(value)));
    CC2520_SPI_TXRX(LO_UINT16(HI_UINT32(value)));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_RXBUF
*
* @brief   Read bytes from RX buffer
*
* @param   u8_t count
*          u8_t  *pData
*
* @return  u8_t - status byte
*/
u8_t CC2520_RXBUF(u8_t count, u8_t  *pData)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_RXBUF);
    CC2520_INS_RD_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_RXBUF8
*
* @brief   Read 8 bits from RX buffer
*
* @param   none
*
* @return  u8_t - result
*/
u8_t CC2520_RXBUF8(void)
{
    u8_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_RXBUF);
    value = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value;
}


/***********************************************************************************
* @fn      CC2520_RXBUF16
*
* @brief   Read 16 bits from RX buffer
*
* @param   none
*
* @return  u16_t - result
*/
u16_t CC2520_RXBUF16(void)
{
    eword_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_RXBUF);
    value.b.b0 = CC2520_SPI_TXRX(0x00);
    value.b.b1 = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value.w;
}


/***********************************************************************************
* @fn      CC2520_RXBUFCP_BEGIN
*
* @brief   Copy RX buf to memory. Call this routine before CC2520_RXBUFCP_END
*
* @param   u16_t addr - copy destination
*          u8_t *pCurrCount - number of bytes in RX buf
*
* @return  u8_t - status byte
*/
u8_t CC2520_RXBUFCP_BEGIN(u16_t addr, u8_t *pCurrCount)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_RXBUFCP);
    if (pCurrCount) {
        *pCurrCount = CC2520_SPI_TXRX(HI_UINT16(addr));
    } else {
        CC2520_SPI_TXRX(HI_UINT16(addr));
    }
    return s;
}


/***********************************************************************************
* @fn      CC2520_RXBUFCP_END
*
* @brief   Copy RX buf to memory and read into buffer.
*          Call this routine after CC2520_RXBUFCP_BEGIN.
*
* @param   u16_t addr - copy destination
*          u8_t count - number of bytes
*          u8_t  *pData - data buffer
*
* @return  u8_t - status byte
*/
u8_t CC2520_RXBUFCP_END(u16_t addr, u8_t count, u8_t  *pData)
{
    u8_t s;
    s = CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_INS_RD_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_RXBUFMOV
*
* @brief   RXBUFMOV instruction - moves oldest bytes from RX buffer to the memory
*          location addr.
*
* @param   u8_t pri - instruction priority
*          u16_t addr - memory location
*          u8_t count - number of bytes
*          u8_t *pCurrCount
*
* @return  u8_t - status byte
*/
u8_t CC2520_RXBUFMOV(u8_t pri, u16_t addr, u8_t count, u8_t *pCurrCount)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_RXBUFMOV | pri);
    if (pCurrCount) {
        *pCurrCount = CC2520_SPI_TXRX(count);
    } else {
        CC2520_SPI_TXRX(count);
    }
    CC2520_SPI_TXRX(HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_TXBUF
*
* @brief   Write data to TX buffer
*
* @param   u8_t count - number of bytes
*          u8_t  *pData - pointer to data buffer
*
* @return  u8_t - status byte
*/
u8_t CC2520_TXBUF(u8_t count, u8_t  *pData)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_TXBUF);
    CC2520_INS_WR_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_TXBUF8
*
* @brief   Write 8 bits to TX buffer
*
* @param   u8_t data
*
* @return  none
*/
void CC2520_TXBUF8(u8_t data)
{
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_TXBUF);
    CC2520_SPI_TXRX(data);
    CC2520_SPI_END();
}


/***********************************************************************************
* @fn      CC2520_TXBUF16
*
* @brief   Write 16 bits to TX buffer
*
* @param   u16_t data
*
* @return  none
*/
void CC2520_TXBUF16(u16_t data)
{
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_TXBUF);
    CC2520_SPI_TXRX(LO_UINT16(data));
    CC2520_SPI_TXRX(HI_UINT16(data));
    CC2520_SPI_END();
}


/***********************************************************************************
* @fn      CC2520_TXBUFCP
*
* @brief   Copy data from memory location addr to TX buf
*
* @param   u8_t pri - priority
*          u16_t addr - memory location
*          u8_t count - number of bytes
*          u8_t *pCurrCount
*
* @return  u8_t - status byte
*/
u8_t CC2520_TXBUFCP(u8_t pri, u16_t addr, u8_t count, u8_t *pCurrCount)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_TXBUFCP | pri);
    if (pCurrCount) {
        *pCurrCount = CC2520_SPI_TXRX(count);
    } else {
        CC2520_SPI_TXRX(count);
    }
    CC2520_SPI_TXRX(HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_RANDOM
*
* @brief   Random generated bytes
*
* @param   u8_t count
*          u8_t  *pData
*
* @return  u8_t - status byte
*/
u8_t CC2520_RANDOM(u8_t count, u8_t  *pData)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_RANDOM);
    CC2520_SPI_TXRX(0x00);
    CC2520_INS_RD_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_RANDOM8
*
* @brief   Random generated byte
*
* @param  none
*
* @return  u8_t
*/
u8_t CC2520_RANDOM8(void)
{
    u8_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_RANDOM);
    CC2520_SPI_TXRX(0x00);
    value = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value;
}


/***********************************************************************************
* @fn      CC2520_RANDOM16
*
* @brief   Random generated bytes
*
* @param  none
*
* @return  u16_t
*/
u16_t CC2520_RANDOM16(void)
{
    eword_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_RANDOM);
    CC2520_SPI_TXRX(0x00);
    value.b.b0 = CC2520_SPI_TXRX(0x00);
    value.b.b1 = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value.w;
}


/***********************************************************************************
* @fn      CC2520_RXMASKOR
*
* @brief   RXMASKOR instruction - bitwise OR between RX enable mask and orMask
*
* @param  u16_t orMask -
*
* @return  u8_t - status byte
*/
u8_t CC2520_RXMASKOR(u16_t orMask)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_RXMASKOR);
    CC2520_SPI_TXRX(HI_UINT16(orMask));
    CC2520_SPI_TXRX(LO_UINT16(orMask));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_RXMASKAND
*
* @brief   RXMASKAND - bitwise AND between RX enable mask and andMask
*
* @param  u16_t andMask
*
* @return  u8_t - status byte
*/
u8_t CC2520_RXMASKAND(u16_t andMask)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_RXMASKAND);
    CC2520_SPI_TXRX(HI_UINT16(andMask));
    CC2520_SPI_TXRX(LO_UINT16(andMask));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_MEMCP
*
* @brief   MEMCP - Copy data between memory blocks
*
* @param  u8_t pri - priority
*         u16_t count - number of bytes
*         u16_t src - source address
*         u16_t dest - destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMCP(u8_t pri, u16_t count, u16_t src, u16_t dest)
{
    return CC2520_INS_MEMCP_COMMON(CC2520_INS_MEMCP, pri, count, src, dest);
}


/***********************************************************************************
* @fn      CC2520_MEMCPR
*
* @brief   MEMCPR - Copy data between memory blocks and revert endianess
*
* @param  u8_t pri - priority
*         u16_t count - number of bytes
*         u16_t src - source address
*         u16_t dest - destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMCPR(u8_t pri, u16_t count, u16_t src, u16_t dest)
{
    return CC2520_INS_MEMCP_COMMON(CC2520_INS_MEMCPR, pri, count, src, dest);
}


/***********************************************************************************
* @fn      CC2520_MEMXCP
*
* @brief   MEMXCP - XOR one memory block with another memory block
*
* @param  u8_t pri - priority
*         u16_t count - number of bytes
*         u16_t src - source address
*         u16_t dest - destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMXCP(u8_t pri, u16_t count, u16_t src, u16_t dest)
{
    return CC2520_INS_MEMCP_COMMON(CC2520_INS_MEMXCP, pri, count, src, dest);
}


/***********************************************************************************
* @fn      CC2520_MEMXWR
*
* @brief   MEMXWR - XOR memory
*
* @param  u16_t addr - memory address
*         u16_t count - number of bytes
*         u8_t  *pData - unaltered data
*
* @return  u8_t - status byte
*/
u8_t CC2520_MEMXWR(u16_t addr, u16_t count, u8_t  *pData)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_MEMXWR);
    CC2520_SPI_TXRX(HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_INS_WR_ARRAY(count, pData);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_BSET
*
* @brief   BSET - set a single bit
*
* @param  u8_t bitAddr - address
*
* @return  u8_t - status byte
*/
u8_t CC2520_BSET(u8_t bitAddr)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_BSET);
    CC2520_SPI_TXRX(bitAddr);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_BCLR
*
* @brief   BCLR - Clear a single bit
*
* @param  u8_t bitAddr - address
*
* @return  u8_t - status byte
*/
u8_t CC2520_BCLR(u8_t bitAddr)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_BCLR);
    CC2520_SPI_TXRX(bitAddr);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_CTR
*
* @brief   CTR - Counter mode encryption
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - number of plaintext bytes
*         u8_t n - address of nonce
*         u16_t src - starting address of plaintext
*         u16_t dest - destination address, if dest=0 destination address is set
*                       equal to src address.
*
* @return  u8_t - status byte
*/
u8_t CC2520_CTR(u8_t pri, u8_t k, u8_t c, u8_t n, u16_t src, u16_t dest)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_CTR | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX(c);
    CC2520_SPI_TXRX(n);
    CC2520_SPI_TXRX((HI_UINT16(src) << 4) | HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_UCTR
*
* @brief   UCTR - Counter mode decryption.
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - number of plaintext bytes
*         u8_t n - address of nonce
*         u16_t src - starting address of ciphertext
*         u16_t dest - destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_UCTR(u8_t pri, u8_t k, u8_t c, u8_t n, u16_t src, u16_t dest)
{
    return CC2520_CTR(pri, k, c, n, src, dest);
}


/***********************************************************************************
* @fn     CC2520_CBCMAC
*
* @brief  CBCMAC - authentication using CBC-MAC
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - number of plaintext bytes
*         u16_t src - starting address of plaintext
*         u16_t dest - destination address
*         u8_t m - sets length of integrity code (m=1,2,3 gives lenght of integrity
*                   field 4,8,16)
*
* @return  u8_t - status byte
*/
u8_t CC2520_CBCMAC(u8_t pri, u8_t k, u8_t c, u16_t src, u16_t dest, u8_t m)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_CBCMAC | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX(c);
    CC2520_SPI_TXRX((HI_UINT16(src) << 4) | HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_TXRX(m);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn     CC2520_UCBCMAC
*
* @brief  UCBCMAC - reverse authentication using CBC-MAC
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - number of plaintext bytes
*         u16_t src - starting address of plaintext
*         u8_t m - sets length of integrity code (m=1,2,3 gives lenght of integrity
*                   field 4,8,16)
*
* @return  u8_t - status byte
*/
u8_t CC2520_UCBCMAC(u8_t pri, u8_t k, u8_t c, u16_t src, u8_t m)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_UCBCMAC | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX(c);
    CC2520_SPI_TXRX(HI_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(m);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_CCM
*
* @brief   CCM - encryption and authentication using CCM
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - number of plaintext bytes to authenticate and encrypt
*         u8_t n - address of nonce
*         u16_t src - starting address of plaintext
*         u16_t dest - destination address
*         u8_t f - number of plaintext bytes to authenticate
*         u8_t m - sets length of integrity code (m=1,2,3 gives lenght of integrity
*                   field 4,8,16)
*
* @return  u8_t - status byte
*/
u8_t CC2520_CCM(u8_t pri, u8_t k, u8_t c, u8_t n, u16_t src, \
    u16_t dest, u8_t f, u8_t m)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_CCM | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX(c);
    CC2520_SPI_TXRX(n);
    CC2520_SPI_TXRX((HI_UINT16(src) << 4) | HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_TXRX(f);
    CC2520_SPI_TXRX(m);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_UCCM
*
* @brief   UCCM - decryption and reverse authentication using CCM
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - number of bytes to decrypt and authenticate
*         u8_t n - address of nonce
*         u16_t src - starting address
*         u16_t dest - destination address
*         u8_t f - number of bytes to authenticate
*         u8_t m - ets length of integrity code (m=1,2,3 gives lenght of integrity
*                   field 4,8,16)
*
* @return  u8_t - status byte
*/
u8_t CC2520_UCCM(u8_t pri, u8_t k, u8_t c, u8_t n, u16_t src, \
    u16_t dest, u8_t f, u8_t m)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_UCCM | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX(c);
    CC2520_SPI_TXRX(n);
    CC2520_SPI_TXRX((HI_UINT16(src) << 4) | HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_TXRX(f);
    CC2520_SPI_TXRX(m);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_ECB
*
* @brief   ECB encryption
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - encrypts 16-C bytes of data in a 16 byte block
*         u16_t src - source address
*         u16_t dest - destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_ECB(u8_t pri, u8_t k, u8_t c, u16_t src, u16_t dest)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_ECB | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX((c << 4) | HI_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_ECBO
*
* @brief   ECB encryption overwriting plaintext
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - encrypts 16-C bytes of data in a 16 byte block
*         u16_t addr - source and destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_ECBO(u8_t pri, u8_t k, u8_t c, u16_t addr)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_ECBO | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX((c << 4) | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_ECBX
*
* @brief   ECB encryption and XOR
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - encrypts 16-C bytes of data in a 16 byte block
*         u16_t src - source address
*         u16_t dest - destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_ECBX(u8_t pri, u8_t k, u8_t c, u16_t src, u16_t dest)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_ECBX | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX((c << 4) | HI_UINT16(src));
    CC2520_SPI_TXRX(LO_UINT16(src));
    CC2520_SPI_TXRX(HI_UINT16(dest));
    CC2520_SPI_TXRX(LO_UINT16(dest));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_ECBXO
*
* @brief   ECBO and XOR
*
* @param  u8_t pri - priority
*         u8_t k - address of key
*         u8_t c - encrypts 16-C bytes of data in a 16 byte block
*         u16_t addr - source and destination address
*
* @return  u8_t - status byte
*/
u8_t CC2520_ECBXO(u8_t pri, u8_t k, u8_t c, u16_t addr)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_ECBXO | pri);
    CC2520_SPI_TXRX(k);
    CC2520_SPI_TXRX((c << 4) | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_INC
*
* @brief   INC - increment instruction. Increments 2^c byte word with least
*                significant byte at address.
*
* @param  u8_t pri - priority
*         u8_t c - increments 2^c byte word
*         u16_t addr - address
*
* @return  u8_t - status byte
*/
u8_t CC2520_INC(u8_t pri, u8_t c, u16_t addr)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_INC | pri);
    CC2520_SPI_TXRX((c << 4) | HI_UINT16(addr));
    CC2520_SPI_TXRX(LO_UINT16(addr));
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_ABORT
*
* @brief   Abort ongoing data management or security instruction
*
* @param  u8_t c - abort mode (see datasheet)
*
* @return  u8_t - status byte
*/
u8_t CC2520_ABORT(u8_t c)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_ABORT);
    CC2520_SPI_TXRX(c);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_REGRD
*
* @brief   Register read. Can only be started from addresses below 0x40
*
* @param  u8_t addr - address
*         u8_t count - number of bytes
*         u8_t  *pValues - buffer to store result
*
* @return  u8_t - status byte
*/
u8_t CC2520_REGRD(u8_t addr, u8_t count, u8_t  *pValues)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_REGRD | addr);
    CC2520_INS_RD_ARRAY(count, pValues);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_REGRD8
*
* @brief   Read one register byte
*
* @param  u8_t addr - address
*
* @return  u8_t - result
*/
u8_t CC2520_REGRD8(u8_t addr)
{
    u8_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_REGRD | addr);
    value = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value;
}


/***********************************************************************************
* @fn      CC2520_REGRD16
*
* @brief   Read two register bytes
*
* @param  u8_t addr - address
*
* @return  u16_t - result
*/
u16_t CC2520_REGRD16(u8_t addr)
{
    eword_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_REGRD | addr);
    value.b.b0 = CC2520_SPI_TXRX(0x00);
    value.b.b1 = CC2520_SPI_TXRX(0x00);
    CC2520_SPI_END();
    return value.w;
}


/***********************************************************************************
* @fn      CC2520_REGRD24
*
* @brief   Read three register bytes
*
* @param  u8_t addr - address
*
* @return  u32_t - result
*/
u32_t CC2520_REGRD24(u8_t addr)
{
    edword_t value;
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_REGRD | addr);
    value.b.b0 = CC2520_SPI_TXRX(0x00);
    value.b.b1 = CC2520_SPI_TXRX(0x00);
    value.b.b2 = CC2520_SPI_TXRX(0x00);
    value.b.b3 = 0x00;
    CC2520_SPI_END();
    return value.dw;
}


/***********************************************************************************
* @fn      CC2520_REGWR
*
* @brief   Register write. Can only be started from addresses below 0x40
*
* @param  u8_t addr - address
*         u8_t count - number of bytes
*         u8_t  *pValues - data buffer
*
* @return  u8_t - status byte
*/
u8_t CC2520_REGWR(u8_t addr, u8_t count, u8_t  *pValues)
{
    u8_t s;
    CC2520_SPI_BEGIN();
    s = CC2520_SPI_TXRX(CC2520_INS_REGWR | addr);
    CC2520_INS_WR_ARRAY(count, pValues);
    CC2520_SPI_END();
    return s;
}


/***********************************************************************************
* @fn      CC2520_REGWR8
*
* @brief   Write one register byte
*
* @param  u8_t addr - address
*         u8_t value
*
* @return  none
*/
void CC2520_REGWR8(u8_t addr, u8_t value)
{
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_REGWR | addr);
    CC2520_SPI_TXRX(value);
    CC2520_SPI_END();
    return;
}


/***********************************************************************************
* @fn      CC2520_REGWR16
*
* @brief   Write two register bytes
*
* @param  u8_t addr - address
*         u16_t value
*
* @return  none
*/
void CC2520_REGWR16(u8_t addr, u16_t value)
{
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_REGWR | addr);
    CC2520_SPI_TXRX(LO_UINT16(value));
    CC2520_SPI_TXRX(HI_UINT16(value));
    CC2520_SPI_END();
}


/***********************************************************************************
* @fn      CC2520_REGWR24
*
* @brief   Write three register bytes
*
* @param  u8_t addr
*         u32_t value
*
* @return  none
*/
void CC2520_REGWR24(u8_t addr, u32_t value)
{
    CC2520_SPI_BEGIN();
    CC2520_SPI_TXRX(CC2520_INS_REGWR | addr);
    CC2520_SPI_TXRX(LO_UINT16(LO_UINT32(value)));
    CC2520_SPI_TXRX(HI_UINT16(LO_UINT32(value)));
    CC2520_SPI_TXRX(LO_UINT16(HI_UINT32(value)));
    CC2520_SPI_END();
}



/***********************************************************************************
  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
***********************************************************************************/

