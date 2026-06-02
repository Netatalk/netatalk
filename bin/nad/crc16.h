#ifndef CRC16_H
#define CRC16_H 1

#include <stddef.h>
#include <stdint.h>

uint16_t crc16_xmodem_update(uint16_t crc, const void *buf, size_t len);

#endif /* CRC16_H */
