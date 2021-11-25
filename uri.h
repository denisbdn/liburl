/* 
 * File:   uri.h
 * Author: bdn
 *
 * Created on 27 Март 2014 г., 19:23
 */

#ifndef URI_H
#define	URI_H

#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define URL_MAX_LENGTH (1024 * 2 * 2 /* UTF-8 string, TODO: maybe use UTF-32 string ? */)
	
#include "punycode.h"
#include "utf8_c.h"
#include "uri_parse.h"
#include "encode.h"

#ifdef	__cplusplus
}
#endif

#endif	/* URI_H */

