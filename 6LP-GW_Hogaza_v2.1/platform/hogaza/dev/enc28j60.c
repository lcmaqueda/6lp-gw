// enc28j60.c: device driver for the ENC28J60 chip.

#include "enc28j60.h"
#include <msp430f5435a.h>

unsigned char Enc28j60Bank;
unsigned int NextPacketPtr;

void _enc28j60Delay(unsigned x){
	for(x; x > 0; x--){
        _nop();
    }
}

void assertCS(void) {
	ENC_CS_PORT &= ~(1<<ENC_CS);			// assert CS
}

void releaseCS(void) {
	ENC_CS_PORT |= (1<<ENC_CS);				// release CS
}

void enc28j60_hardReset(void) {
	ENC_RESET_PORT &= ~(1<<ENC_RESET);		// perform hard reset
	_enc28j60Delay(18000);
	ENC_RESET_PORT |= (1<<ENC_RESET);		// release reset
}

void init_spi(void) {

	UCB0CTL1  = UCSWRST;				// Put USART0 in reset mode
	// Configure I/O pins
	ENC_CS_DIR  |= (1 << ENC_CS);		// Set P5.3 as CS (output pin)
	ENC_CS_PORT |= (1 << ENC_CS);		// Set CS high
	ENC_SCK_DIR |= (1 << ENC_SCK); 		// Set SCK as output
	ENC_SPI_DIR |= (1 << ENC_MOSI);		// Set MOSI as output
	ENC_SPI_DIR &= ~(1 << ENC_MISO);	// Set MISO as input
	ENC_SPI_SEL |= (1 << ENC_MOSI) + (1 << ENC_MISO);	// Special functions for SPI pins
	ENC_SCK_SEL |= (1 << ENC_SCK); 		// Special functions for SCK pin
	ENC_SPI_PORT = 0x00;

	// Set SPI registers (see slau144e.pdf for reference)
	UCB0CTL0 = (UCSYNC | UCMST | UCMSB | UCCKPH);	// Set master, synchronous,
											// 8-bit, msb, 3-pin spi mode
	UCB0CTL1 |= UCSSEL_2;      				// SMCLK
	UCB0BR0  = 0x04;						// Baud rate 0 (SMCLK/4, 8MHz) 
	UCB0BR1  = 0x00;						// Baud rate 1 (upper 16 bit of baud rate divisor)
	//UCB0CTL = 0x00;						// Modulation control (no modulation in SPI!) (needed?--> Should be cleared only for USCI_A)
	UCB0CTL1 &= ~UCSWRST;					// Deactivate reset state
}

unsigned char spi_rw_byte(unsigned char data) {
	while((UCB0IFG & UCTXIFG) == 0);    	// wait until TX buffer empty
	UCB0TXBUF = data;						// send byte
  	while((UCB0IFG & UCRXIFG) == 0) {}		// data present in RX buffer?
	return UCB0RXBUF;						// return read data
}

unsigned char enc28j60ReadOp(unsigned char op, unsigned char address) {
	unsigned char data = 0;
	assertCS();							// assert CS signal

	spi_rw_byte(op | (address & ADDR_MASK));	// issue read command
	data = spi_rw_byte(0);				// read data (send zeroes)

	if(address & 0x80) {				// do dummy read if needed
		data = spi_rw_byte(0);			// read data (send zeroes)
	}

	releaseCS();						// release CS signal
	return data;
}

void enc28j60WriteOp(unsigned char op, unsigned char address, unsigned char data) {
	assertCS();
	spi_rw_byte(op | (address & ADDR_MASK));// issue write command
	if (op != ENC28J60_SOFT_RESET) spi_rw_byte(data);	// send data
    	releaseCS();
}


void enc28j60ReadBuffer(unsigned int len, unsigned char* data) {
	assertCS();

	spi_rw_byte(ENC28J60_READ_BUF_MEM);		// issue read command
	while(len--) {
		*data++ = spi_rw_byte(0);
	}
	releaseCS();
}


void enc28j60WriteBuffer(unsigned int len, unsigned char* data) {
	assertCS();

	spi_rw_byte(ENC28J60_WRITE_BUF_MEM);		// issue write command
	while(len--) {
		spi_rw_byte(*data++);
	}
	releaseCS();
}

void enc28j60SetBank(unsigned char address) {
	if((address & BANK_MASK) != Enc28j60Bank) {	// set the bank (if needed)
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

unsigned char enc28j60Read(unsigned char address) {
	enc28j60SetBank(address);									// set the bank
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);		// do the read
}


void enc28j60Write(unsigned char address, unsigned char data) {
	enc28j60SetBank(address);									// set the bank
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);	// do the write
}

unsigned int enc28j60PhyRead(unsigned char address) {

	enc28j60Write(MIREGADR, address);		// set the PHY register address
	enc28j60Write(MICMD, MICMD_MIIRD);

	while(enc28j60Read(MISTAT) & MISTAT_BUSY);	// wait until the PHY read completes

	enc28j60Write(MICMD, 0);	// Clear MICMD.MIIRD bit
	return ((enc28j60Read(MIRDH) << 8) | enc28j60Read(MIRDL));
}

void enc28j60PhyWrite(unsigned char address, unsigned int data) {
	enc28j60Write(MIREGADR, address);		// set the PHY register address
	enc28j60Write(MIWRL, data);				// write the PHY data
	enc28j60Write(MIWRH, data>>8);

	while(enc28j60Read(MISTAT) & MISTAT_BUSY);	// wait until the PHY write completes
}

int enc28j60_init(void) {
	init_spi();
	// set output clock disabled
	enc28j60Write(ECOCON, 0x00);
	// perform system reset
	// Suggested Workaround for errata#1 (wait 1ms)
	// as XT2 freq is 16MHz, waiting 16000 cicles (aprox) will do the job.
	// Improvement of the workaround achieved by means of a hard reset.
	//enc28j60_hardReset();
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET); 
	_enc28j60Delay(18000); // no longer needed
	// check CLKRDY bit to see if reset is complete
	while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// Set LED configuration
	//enc28j60PhyWrite(PHLCON, 0x3ba6);

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// ERXWRPT set automatically when ERXST and ERXND set
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address (-1 due to errata #11)
	if ((NextPacketPtr - 1 < RXSTART_INIT) ||
			(NextPacketPtr - 1 > RXSTOP_INIT)) {
		enc28j60Write(ERXRDPTL, (RXSTOP_INIT)&0xFF);
		enc28j60Write(ERXRDPTH, (RXSTOP_INIT)>>8);
	} else { // 
		enc28j60Write(ERXRDPTL, (RXSTART_INIT-1)&0xFF);
		enc28j60Write(ERXRDPTH, (RXSTART_INIT-1)>>8);
	}
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// set buffer read pointer
	enc28j60Write(ERDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERDPTH, RXSTART_INIT>>8);
	// set transmit buffer start
	// ETXST defaults to 0x0000 (beginnging of ram)
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);
	// set end of transmit buffer
	enc28j60Write(ETXNDL, TXEND_INIT&0xFF);
	enc28j60Write(ETXNDH, TXEND_INIT>>8);
	// disable filter out of multicast packets
	//enc28j60Write(ERXFCON, 0xA3);	
	// All packet with a valid CRC will be accepted.	
	enc28j60Write(ERXFCON, 0x20);
	
	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// enable automatic padding and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set MACON 4 bits (necessary for half-duplex only)
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON4, MACON4_DEFER);

	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);

	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR6, ENC28J60_MAC5);
	enc28j60Write(MAADR5, ENC28J60_MAC4);
	enc28j60Write(MAADR4, ENC28J60_MAC3);
	enc28j60Write(MAADR3, ENC28J60_MAC2);
	enc28j60Write(MAADR2, ENC28J60_MAC1);
	enc28j60Write(MAADR1, ENC28J60_MAC0);

	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	return 1;
}


void enc28j60PacketSend(unsigned int len, unsigned char* packet) {

	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT&0xFF);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);

	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len)&0xFF);
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// workaround due to errata#10
	// perform transmit only reset
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	// wait until transmission ends
	while(enc28j60Read(ECON1) & ECON1_TXRST);
#if UIP_LOGGING
	// check if there was an error
	if(enc28j60Read(ESTAT) & ESTAT_TXABRT){
		// TODO: workaround for errata#13 (read_TSV)
		
	} else {
		
	}
#endif
}

int enc28j60_pending_packet() {
	return enc28j60Read(EPKTCNT);
}

unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char* packet) {
	unsigned int rxstat;
	unsigned int len;

	// check if a packet has been received and buffered
	if(!enc28j60Read(EPKTCNT)){
		return 0;
	}

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr)&0xFF);
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	//debug
	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	len = MIN(len, maxlen);

	// copy the packet from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	// (workaround is needed due to errata #11)
	if ((NextPacketPtr - 1 < RXSTART_INIT) ||
			(NextPacketPtr - 1 > RXSTOP_INIT)){
		enc28j60Write(ERXRDPTL, (RXSTOP_INIT)&0xFF);
		enc28j60Write(ERXRDPTH, (RXSTOP_INIT)>>8);
	} else {
        enc28j60Write(ERXRDPTL, (NextPacketPtr-1)&0xFF);
        enc28j60Write(ERXRDPTH, (NextPacketPtr-1)>>8);	
	}

		// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return len;
}

unsigned long int enc28j60BlinkLeds(unsigned long int interval, unsigned char times){
	unsigned int old;
	unsigned long int i;
	unsigned char j;
	old = enc28j60PhyRead(PHLCON);
	// turn leds off
	enc28j60PhyWrite(PHLCON, 0x3882);
	// make led A blink slow, B fast
	enc28j60PhyWrite(PHLCON, 0x3BA2);
	for(j = 0; j < times; j++){
		// wait interval
		for(i = 0; i < interval; i++){
			_nop();
			_nop();
		}
	}
	//restore PHLCON
	enc28j60PhyWrite(PHLCON, old);
	return old;
}

void read_TSV(unsigned char *tsv){
	unsigned int read_pt;
	read_pt = enc28j60Read(ETXNDL);
	read_pt |= enc28j60Read(ETXNDH)<<8;
	enc28j60Write(ERDPTL, (read_pt+1)&0xFF);
	enc28j60Write(ERDPTH, (read_pt+1)>>8);
	enc28j60ReadBuffer(7, tsv);
}
