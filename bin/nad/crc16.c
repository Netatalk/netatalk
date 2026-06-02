/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "crc16.h"

#define CRC16_XMODEM_POLY 0x1021u

uint16_t crc16_xmodem_update(uint16_t crc, const void *buf, size_t len)
{
    const uint8_t *ptr = buf;

    while (len-- > 0) {
        crc ^= (uint16_t) * ptr++ << 8;

        for (unsigned int bit = 0; bit < 8; bit++) {
            if ((crc & 0x8000) != 0) {
                crc = (uint16_t)((crc << 1) ^ CRC16_XMODEM_POLY);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}
