/* 
 * File:   utf8_c.h
 * Author: bdn
 *
 * Created on 12 Март 2014 г., 19:15
 */

#ifndef UTF8_C_H
#define	UTF8_C_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

int sequence_length(const uint8_t *lead_it);
char* append(uint32_t c, char *utf8);
uint32_t next(const char ** utf8);

#ifdef	__cplusplus
}
#endif

#endif	/* UTF8_C_H */

