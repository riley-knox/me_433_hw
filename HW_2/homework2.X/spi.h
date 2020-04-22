#ifndef SPI__H__
#define SPI__H__

void initSPI();
unsigned char spi_io(unsigned char o);
unsigned short create_spi_write(unsigned char channel, unsigned short voltage);

#endif // SPI__H__