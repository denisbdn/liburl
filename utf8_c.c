#include "utf8_c.h"

#define MASK8(x) (0xff & (x))

int sequence_length(const uint8_t *lead_it) {
	uint8_t lead = MASK8(*lead_it);
	if (lead < 0x80)
		return 1;
	else if ((lead >> 5) == 0x6)
		return 2;
	else if ((lead >> 4) == 0xe)
		return 3;
	else if ((lead >> 3) == 0x1e)
		return 4;
	else
		return 0;
}

char* append(uint32_t c, char *utf8) {
	uint8_t *ptr = (uint8_t*) utf8;
	if (c < 0x80) {
		*ptr++ = c;
	} else if (c < 0x800) {
		ptr[0] = 0xc0 | (c >> 6);
		ptr[1] = 0x80 | (c & 0x3f);
		ptr += 2;
	} else {
		ptr[0] = 0xf0 | (c >> 12);
		ptr[1] = 0x80 | ((c >> 6) & 0x3f);
		ptr[2] = 0x80 | (c & 0x3f);
		ptr += 3;
	}
	return (char*) ptr;
}

uint32_t next(const char ** utf8) {
	uint32_t cp = MASK8(*(*utf8));
	int length = sequence_length((const uint8_t *)(*utf8));
	switch (length) {
		case 1:
			break;
		case 2:
			(*utf8)++;
			cp = ((cp << 6) & 0x7ff) + ((*(*utf8)) & 0x3f);
			break;
		case 3:
			++(*utf8);
			cp = ((cp << 12) & 0xffff) + ((MASK8(*(*utf8)) << 6) & 0xfff);
			++(*utf8);
			cp += (*(*utf8)) & 0x3f;
			break;
		case 4:
			++(*utf8);
			cp = ((cp << 18) & 0x1fffff) + ((MASK8(*(*utf8)) << 12) & 0x3ffff);
			++(*utf8);
			cp += (MASK8(*(*utf8)) << 6) & 0xfff;
			++(*utf8);
			cp += (*(*utf8)) & 0x3f;
			break;
	}
	++(*utf8);
	return cp;
}
