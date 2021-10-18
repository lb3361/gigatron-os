#ifndef SPI_H
#define SPI_H

extern void spi_send(const void *buffer, int len);
extern void spi_recv(void *buffer, int len);

#endif
