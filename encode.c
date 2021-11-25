
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "encode.h"
#include "utf8_c.h"
#include "uri_parse.h"
#include "punycode.h"

#define REINDEX_PATH(path, new_str, old_str) \
if ((path).begin) (path).begin = (new_str) + ((path).begin - (old_str)); \
if ((path).end) (path).end = (new_str) + ((path).end - (old_str));

static inline int realloc_holder(struct holder *h, size_t s)
{
	if (h->size > s)
		return 1;
	char *new_str = realloc(h->data.begin, s);
	if (!new_str) {
		return 0;
	} else if (new_str == h->data.begin) {
		if (s > h->size)
			memset(new_str + h->size, 0, s - h->size);
		else if (s < h->size)
			memset(new_str + s, 0, h->size - s);
		h->size = s;
	} else {
		if (s > h->size)
			memset(new_str + h->size, 0, s - h->size);
		else if (s < h->size)
			memset(new_str + s, 0, h->size - s);
		REINDEX_PATH(h->uri_desc.scheme, new_str, h->data.begin);
		REINDEX_PATH(h->uri_desc.userinfo, new_str, h->data.begin);
		REINDEX_PATH(h->uri_desc.host, new_str, h->data.begin);
		REINDEX_PATH(h->uri_desc.port, new_str, h->data.begin);
		REINDEX_PATH(h->uri_desc.path, new_str, h->data.begin);
		REINDEX_PATH(h->uri_desc.query, new_str, h->data.begin);
		REINDEX_PATH(h->uri_desc.fragment, new_str, h->data.begin);
		h->data.end = new_str + (h->data.end - h->data.begin);
		h->size = s;
		h->data.begin = new_str;
	}
	return 1;
}

int is_empty_holder(const struct holder *h)
{
	struct holder cmp;
	ZERROSET(cmp);
	int res = memcmp(h, &cmp, sizeof(cmp));
	return res == 0 ? 1 : 0;
}

struct holder *free_holder(struct holder *h)
{
	free(h->data.begin);
	memset(h, 0, sizeof(*h));
	return h;
}

#define ADD_BYTES 64	// > 2 > 4

#define ADD_HEX(h, c) { \
	if (((h).size - ((h).data.end - (h).data.begin)) < 4) realloc_holder(&(h), (h).size + ADD_BYTES); \
	(h).data.end += sprintf((h).data.end, "%%%.2hhX", (unsigned char)(c)); \
}

#define ADD_CHAR(h, c) { \
	if (((h).size - ((h).data.end - (h).data.begin)) < 2) realloc_holder(&(h), (h).size + ADD_BYTES); \
	(h).data.end += sprintf((h).data.end, "%c", (char)(c)); \
}

typedef enum url_type {
	NONE,
	UNRESERVED,
	HEX,
	RESERVED_GEN,
	RESERVED_SUB,
} url_type_t;

#define IF_LOW_ALFABET_NUM(s) (((s) >= (int)'a' && (s) <= (int)'z') ? 1 : 0)
#define IF_APPER_ALFABET_NUM(s) (((s) >= (int)'A' && (s) <= (int)'Z') ? 1 : 0)
#define IF_DIGIT_NUM(s) (((s) >= (int)'0' && (s) <= (int)'9') ? 1 : 0)

static inline url_type_t find_symbol_type_num(uint32_t symbol)
{
	if (IF_LOW_ALFABET_NUM(symbol))
		return UNRESERVED;
	if (IF_APPER_ALFABET_NUM(symbol))
		return UNRESERVED;
	if (IF_DIGIT_NUM(symbol))
		return UNRESERVED;
	switch (symbol) {
		case (int)'-':
		case (int)'.':
		case (int)'_':
		case (int)'~':
			return UNRESERVED;
		case (int)'%':
			return HEX;
		case (int)':':
		case (int)'/':
		case (int)'?':
		case (int)'#':
		case (int)'[':
		case (int)']':
		case (int)'@':
			return RESERVED_GEN;
		case (int)'!':
		case (int)'$':
		case (int)'&':
		case (int)'\'':
		case (int)'(':
		case (int)')':
		case (int)'*':
		case (int)'+':
		case (int)',':
		case (int)';':
		case (int)'=':
			return RESERVED_SUB;
		default:
			return NONE;
	}
}

#define IF_LOW_ALFABET_CHAR(s) (((s) >= 'a' && (s) <= 'z') ? 1 : 0)
#define IF_APPER_ALFABET_CHAR(s) (((s) >= 'A' && (s) <= 'Z') ? 1 : 0)
#define IF_DIGIT_CHAR(s) (((s) >= '0' && (s) <= '9') ? 1 : 0)

static inline char to_upper(char c)
{
	if (c >= 'a' && c <= 'z') {
		return ('A' + (c - 'a'));
	} else {
		return c;
	}
}

static void to_upper_encode(struct holder *h)
{
	if (!h || !(h->data.begin) || !(h->data.end))
		return;
	char *curr;
	for (curr = h->data.begin; curr < h->data.end; ) {
		if (*curr == '%') {
			char *first = curr + 1;
			char *second = curr + 2;
			if (first >= h->data.end || second >= h->data.end)
				++curr;
			else {
				if (IF_LOW_ALFABET_CHAR(*first) && IF_LOW_ALFABET_CHAR(*second)) {
					*first = to_upper(*first);
					*second = to_upper(*second);
					curr += 2;
				} else if (IF_APPER_ALFABET_CHAR(*first) && IF_LOW_ALFABET_CHAR(*second)) {
					*second = to_upper(*second);
					curr += 2;
				} else if (IF_LOW_ALFABET_CHAR(*first) && IF_APPER_ALFABET_CHAR(*second)) {
					*first = to_upper(*first);
					curr += 2;
				} else if (IF_APPER_ALFABET_CHAR(*first) && IF_APPER_ALFABET_CHAR(*second)) {
					curr += 2;
				} else {
					++curr;
				}
			}
		} else {
			++curr;
		}
	}
	return;
}

// scheme://[[userinfo]@]foo.com[:port]]/[path][?query][#fragment]
// [path][?query][#fragment]
struct holder encode(const struct uri *source)
{
	const char *curr;
	char symbol;
	struct holder h;
	ZERROSET(h);
	if (source->scheme.begin && source->scheme.end && source->scheme.begin != source->scheme.end) {
		for (curr = source->scheme.begin; curr < source->scheme.end; ++curr) {
			if (sequence_length((const uint8_t *)curr) != 1) {
				free_holder(&h);
				return h;
			}
			symbol = *curr;
			if (curr == source->scheme.begin) {
				if (IF_LOW_ALFABET_CHAR(symbol) || IF_APPER_ALFABET_CHAR(symbol)) {
					ADD_CHAR(h, symbol);
					h.uri_desc.scheme.begin = h.data.end - 1;
				} else {
					free_holder(&h);
					return h;
				}
			} else {
				if (
						IF_LOW_ALFABET_CHAR(symbol) ||
						IF_APPER_ALFABET_CHAR(symbol) ||
						IF_DIGIT_CHAR(symbol) ||
						symbol == '+' ||
						symbol == '-' ||
						symbol == '.'
				) {
					ADD_CHAR(h, symbol);
				} else {
					free_holder(&h);
					return h;
				}
			}
		}
		ADD_CHAR(h, ':');
		h.uri_desc.scheme.end = h.data.end - 1;
		ADD_CHAR(h, '/');
		ADD_CHAR(h, '/');
	}
	if (source->userinfo.begin && source->userinfo.end) {
		for (curr = source->userinfo.begin; curr < source->userinfo.end;) {
			const char *tmp = curr;
			int len = sequence_length((const uint8_t *)tmp);
			uint32_t number = next(&tmp);
			int i;
			switch(find_symbol_type_num(number)) {
				case NONE:
					for (i = 0; i < len; ++i, ++curr) {
						ADD_HEX(h, *curr);
						if (curr == source->userinfo.begin)
							h.uri_desc.userinfo.begin = h.data.end - 3;
					}
					break;
				case UNRESERVED:
				case HEX:
				case RESERVED_SUB:
					ADD_CHAR(h, *curr);
					if (curr == source->userinfo.begin)
						h.uri_desc.userinfo.begin = h.data.end - 1;
					++curr;
					break;
				case RESERVED_GEN:
					if (*curr == ':') {
						ADD_CHAR(h, *curr);
						if (curr == source->userinfo.begin)
							h.uri_desc.userinfo.begin = h.data.end - 1;
						++curr;
						break;
					} 
				default:
					free_holder(&h);
					return h;
			}
		}
		ADD_CHAR(h, '@');
		h.uri_desc.userinfo.end = h.data.end - 1;
	}
	if (source->host.begin && source->host.end && source->host.begin != source->host.end) {
		char out[MAX_DOMAIN_NAME * 6 + 1];
		const char *host = from_utf8_to_punycode_reent_(source->host.begin, source->host.end - source->host.begin, out, sizeof(out));
		size_t host_length;
		if (host && (host_length = strlen(host))) {
			if ( h.size - (h.data.end - h.data.begin) < host_length + 1)
				realloc_holder(&h, h.size + (host_length + 1) + (ADD_BYTES - (host_length + 1)%ADD_BYTES));
			h.uri_desc.host.begin = h.data.end;
			strncpy(h.data.end, host, host_length);
			h.data.end += host_length;
			h.uri_desc.host.end = h.data.end;
		}
	}
	if (source->port.begin && source->port.end && source->port.begin != source->port.end) {
		ADD_CHAR(h, ':');
		for (curr = source->port.begin; curr < source->port.end;) {
			if (IF_DIGIT_CHAR(*curr)) {
				if (curr == source->port.begin) {
					ADD_CHAR(h, *curr);
					h.uri_desc.port.begin = h.data.end - 1;
				} else
					ADD_CHAR(h, *curr);
				++curr;
			} else {
				free_holder(&h);
				return h;
			}
		}
		h.uri_desc.port.end = h.data.end;
	}
	h.uri_desc.has_begin_path_slash = source->has_begin_path_slash;
	if (h.uri_desc.has_begin_path_slash)
		ADD_CHAR(h, '/');
	if (source->path.begin && source->path.end && source->path.begin != source->path.end) {
		for (curr = source->path.begin; curr < source->path.end;) {
			const char *tmp = curr;
			int len = sequence_length((const uint8_t *)tmp);
			uint32_t number = next(&tmp);
			int i;
			switch(find_symbol_type_num(number)) {
				case NONE:
					for (i = 0; i < len; ++i, ++curr) {
						ADD_HEX(h, *curr);
						if (curr == source->path.begin)
							h.uri_desc.path.begin = h.data.end - 3;
					}
					break;
				case UNRESERVED:
				case HEX:
				case RESERVED_SUB:
					ADD_CHAR(h, *curr);
					if (curr == source->path.begin)
						h.uri_desc.path.begin = h.data.end - 1;
					++curr;
					break;
				case RESERVED_GEN:
					if (*curr == '?' || *curr == '#') {
						free_holder(&h);
						return h;
					} else if (*curr == '/') {
						ADD_CHAR(h, *curr);
						if (curr == source->path.begin)
							h.uri_desc.path.begin = h.data.end - 1;
						++curr;
						break;
					} else {
						ADD_HEX(h, *curr);
						if (curr == source->path.begin)
							h.uri_desc.path.begin = h.data.end - 1;
						++curr;
						break;
					}
				default:
					free_holder(&h);
					return h;
			}
		}
		h.uri_desc.path.end = h.data.end;
	}
	h.uri_desc.has_end_path_slash = source->has_end_path_slash;
	if (source->query.begin && source->query.end && source->query.begin != source->query.end) {
		ADD_CHAR(h, '?');
		for (curr = source->query.begin; curr < source->query.end;) {
			const char *tmp = curr;
			int len = sequence_length((const uint8_t *)tmp);
			uint32_t number = next(&tmp);
			int i;
			switch(find_symbol_type_num(number)) {
				case NONE:
					for (i = 0; i < len; ++i, ++curr) {
						ADD_HEX(h, *curr);
						if (curr == source->query.begin)
							h.uri_desc.query.begin = h.data.end - 3;
					}
					break;
				case UNRESERVED:
				case HEX:
				case RESERVED_SUB:
					ADD_CHAR(h, *curr);
					if (curr == source->query.begin)
						h.uri_desc.query.begin = h.data.end - 1;
					++curr;
					break;
				case RESERVED_GEN:
					if (*curr == '#') {
						free_holder(&h);
						return h;
					}
					ADD_CHAR(h, *curr);
					if (curr == source->query.begin)
						h.uri_desc.query.begin = h.data.end - 1;
					++curr;
					break;
				default:
					free_holder(&h);
					return h;
			}
		}
		h.uri_desc.query.end = h.data.end;
	}
	if (source->fragment.begin && source->fragment.end && source->fragment.begin != source->fragment.end) {
		ADD_CHAR(h, '#');
		for (curr = source->fragment.begin; curr < source->fragment.end;) {
			const char *tmp = curr;
			int len = sequence_length((const uint8_t *)tmp);
			uint32_t number = next(&tmp);
			int i;
			switch(find_symbol_type_num(number)) {
				case NONE:
					for (i = 0; i < len; ++i, ++curr) {
						ADD_HEX(h, *curr);
						if (curr == source->fragment.begin)
							h.uri_desc.fragment.begin = h.data.end - 3;
					}
					break;
				case UNRESERVED:
				case HEX:
				case RESERVED_SUB:
					ADD_CHAR(h, *curr);
					if (curr == source->fragment.begin)
						h.uri_desc.fragment.begin = h.data.end - 1;
					++curr;
					break;
				case RESERVED_GEN:
					ADD_CHAR(h, *curr);
					if (curr == source->fragment.begin)
						h.uri_desc.fragment.begin = h.data.end - 1;
					++curr;
					break;
				default:
					free_holder(&h);
					return h;
			}
		}
		h.uri_desc.fragment.end = h.data.end;
	}
	to_upper_encode(&h);
	return h;
}

static char *strmove(char *dest, char *source)
{
	while (*source)
		*dest++ = *source++;
	*dest = '\0';
	return dest;
}

static char *strrchrfrom(char *begin, char *end, char found)
{
	for ( ; end >= begin; --end)
		if (*end == found)
			return end;
	return NULL;
}

static size_t remove_dot(char *path) {
	size_t res = 0;
	char *curr_dest, *curr_source;
	// первый проход удаляем // и /./
	curr_dest = path;
	curr_source = path;
	while (*curr_source) {
		if (!strncmp(curr_source, "//", 2)) {
			curr_source += 1;
			res += 1;
		} else if (!strncmp(curr_source, "/./", 3)) {
			curr_source += 2;
			res += 2;
		} else {
			*curr_dest++ = *curr_source++;
		}
	}
	*curr_dest = '\0';
	// второй проход удаляем /../
	curr_source = strstr(path, "/../");
	while (curr_source) {
		curr_dest = strrchrfrom(path, curr_source - 1, '/');
		if (curr_dest) {
			strmove(curr_dest + 1, curr_source + 4);
			res += (curr_source + 4) - (curr_dest + 1);
			curr_source = strstr(curr_dest, "/../");
		} else {
			strmove(path, curr_source + 3);
			res += (curr_source + 3) - path;
			curr_source = strstr(path, "/../");
		}
	}
	return res;
}

struct holder relative_uri(const struct uri *base, const struct uri *relative)
{
	struct uri target;
	ZERROSET(target);
	struct path added_path;
	ZERROSET(added_path);
	int added_has_begin_path_slash = 0;
	int added_has_end_path_slash = 0;
	if (relative->scheme.begin && relative->scheme.end) {
		target.scheme = relative->scheme;
		target.userinfo = relative->userinfo;
		target.host = relative->host;
		target.port = relative->port;
		target.path = relative->path;
		target.query = relative->query;
		target.has_begin_path_slash = relative->has_begin_path_slash;
		target.has_end_path_slash = relative->has_end_path_slash;
	} else {
		if (relative->userinfo.begin && relative->userinfo.end) {
			target.userinfo = relative->userinfo;
			target.host = relative->host;
			target.port = relative->port;
			target.path = relative->path;
			target.query = relative->query;
			target.has_begin_path_slash = relative->has_begin_path_slash;
			target.has_end_path_slash = relative->has_end_path_slash;
		} else {
			if (relative->host.begin && relative->host.end) {
				target.host = relative->host;
				target.port = relative->port;
				target.path = relative->path;
				target.query = relative->query;
				target.has_begin_path_slash = relative->has_begin_path_slash;
				target.has_end_path_slash = relative->has_end_path_slash;
			} else {
				if (relative->port.begin && relative->port.end) {
					target.port = relative->port;
					target.path = relative->path;
					target.query = relative->query;
					target.has_begin_path_slash = relative->has_begin_path_slash;
					target.has_end_path_slash = relative->has_end_path_slash;
				} else {
					if (relative->path.begin && relative->path.end) {
						if (relative->has_begin_path_slash) {
							target.path = relative->path;
							target.query = relative->query;
							target.has_begin_path_slash = relative->has_begin_path_slash;
							target.has_end_path_slash = relative->has_end_path_slash;
						} else {
							target.path = base->path;
							added_path = relative->path;
							target.query = relative->query;
							target.has_begin_path_slash = base->has_begin_path_slash;
							added_has_begin_path_slash = relative->has_begin_path_slash;
							target.has_end_path_slash = base->has_end_path_slash;
							added_has_end_path_slash = relative->has_end_path_slash;
						}
					} else {
						if (relative->query.begin && relative->query.end) {
							target.query = relative->query;
						} else {
							target.query = base->query;
						}
						target.has_begin_path_slash = base->has_begin_path_slash;
						target.has_end_path_slash = base->has_end_path_slash;
						target.path = base->path;
					}
					//target.port = base->port;
				}
				target.host = base->host;
			}
			target.userinfo = base->userinfo;
		}
		target.scheme = base->scheme;
	}
	target.fragment = relative->fragment;
	struct holder h;
	ZERROSET(h);
	if (
			!target.scheme.begin || !target.scheme.end || target.scheme.begin == target.scheme.end ||
			!target.host.begin || !target.host.end || target.host.begin == target.host.end
	)
		return h;
	size_t s =	(target.scheme.end - target.scheme.begin) +
				3 + //	"://"
				(target.userinfo.end - target.userinfo.begin) +
				1 + //	"@"
				(target.host.end - target.host.begin) +
				1 + //	":"
				(target.port.end - target.port.begin) +
				1 + //	"/"
				(target.path.end - target.path.begin) +
				1 + //	"/"
				(added_path.end - added_path.begin) +
				1 + //	"?"
				(target.query.end - target.query.begin) +
				1 + //	"#"
				(target.fragment.end - target.fragment.begin) +
				1; //	"\0"
	realloc_holder(&h, s);
	if (target.scheme.begin && target.scheme.end) {
		memcpy(h.data.end, target.scheme.begin, target.scheme.end - target.scheme.begin);
		h.uri_desc.scheme.begin = h.data.end;
		h.data.end += target.scheme.end - target.scheme.begin;
		h.uri_desc.scheme.end = h.data.end;
		memcpy(h.data.end, "://", 3);
		h.data.end += 3;
	}
	if (target.userinfo.begin && target.userinfo.end) {
		memcpy(h.data.end, target.userinfo.begin, target.userinfo.end - target.userinfo.begin);
		h.uri_desc.userinfo.begin = h.data.end;
		h.data.end += target.userinfo.end - target.userinfo.begin;
		h.uri_desc.userinfo.end = h.data.end;
		memcpy(h.data.end, "@", 1);
		h.data.end += 1;
	}
	if (target.host.begin && target.host.end) {
		memcpy(h.data.end, target.host.begin, target.host.end - target.host.begin);
		h.uri_desc.host.begin = h.data.end;
		h.data.end += target.host.end - target.host.begin;
		h.uri_desc.host.end = h.data.end;
	}
	if (target.path.begin && target.path.end) {
		memcpy(h.data.end, "/", 1);
		h.data.end += 1;
		memcpy(h.data.end, target.path.begin, target.path.end - target.path.begin);
		h.uri_desc.path.begin = h.data.end;
		h.data.end += target.path.end - target.path.begin;
		h.uri_desc.path.end = h.data.end;
		h.uri_desc.has_begin_path_slash = 1;
	}
	if (added_path.begin && added_path.end) {
		const char *t = NULL;
		for (t = h.uri_desc.path.end - 1; t > h.uri_desc.path.begin; --t) {
			if (*t == '/')
				break;
		}
		if (t != h.uri_desc.path.begin) {
			h.data.end = (char *)t;
			h.uri_desc.path.end = t;
		}
		memcpy(h.data.end, "/", 1);
		h.data.end += 1;
		memcpy(h.data.end, added_path.begin, added_path.end - added_path.begin);
		if (!h.uri_desc.path.begin)
			h.uri_desc.path.begin = h.data.end;
		h.data.end += added_path.end - added_path.begin;
		h.uri_desc.path.end = h.data.end;
		h.uri_desc.has_begin_path_slash = 1;
	}
	if (h.uri_desc.path.begin && h.uri_desc.path.end) {
		*((char *)h.uri_desc.path.end) = '\0';
		size_t s = remove_dot((char *)(h.uri_desc.path.begin - 1));
		h.uri_desc.path.end -= s;
		if (h.uri_desc.path.begin != h.uri_desc.path.end && *(h.uri_desc.path.end - 1) == '/')
			h.uri_desc.has_end_path_slash = 1;
	}
	if (target.query.begin && target.query.end) {
		memcpy(h.data.end, "?", 1);
		h.data.end += 1;
		memcpy(h.data.end, target.query.begin, target.query.end - target.query.begin);
		h.uri_desc.query.begin = h.data.end;
		h.data.end += target.query.end - target.query.begin;
		h.uri_desc.query.end = h.data.end;
		h.uri_desc.has_begin_path_slash = 1;
	}
	if (target.fragment.begin && target.fragment.end) {
		memcpy(h.data.end, "#", 1);
		h.data.end += 1;
		memcpy(h.data.end, target.fragment.begin, target.fragment.end - target.fragment.begin);
		h.uri_desc.fragment.begin = h.data.end;
		h.data.end += target.fragment.end - target.fragment.begin;
		h.uri_desc.fragment.end = h.data.end;
		h.uri_desc.has_begin_path_slash = 1;
	}
	return h;
}

const char *to_string(struct holder *source)
{
	if (!source || !source->data.begin || !source->data.end)
		return NULL;
	if (*(source->data.end) != '\0')
		ADD_CHAR(*source, '\0');
	return source->data.begin;
}

void copy(struct holder *dest, const struct holder *source)
{
	if (source->size && source->data.begin && source->data.end) {
		*dest = *source;
		dest->data.begin = malloc(source->size);
		memcpy(dest->data.begin, source->data.begin, source->size);
		REINDEX_PATH(dest->uri_desc.scheme, dest->data.begin, source->data.begin);
		REINDEX_PATH(dest->uri_desc.userinfo, dest->data.begin, source->data.begin);
		REINDEX_PATH(dest->uri_desc.host, dest->data.begin, source->data.begin);
		REINDEX_PATH(dest->uri_desc.port, dest->data.begin, source->data.begin);
		REINDEX_PATH(dest->uri_desc.path, dest->data.begin, source->data.begin);
		REINDEX_PATH(dest->uri_desc.query, dest->data.begin, source->data.begin);
		REINDEX_PATH(dest->uri_desc.fragment, dest->data.begin, source->data.begin);
	}
}

int defaultPort(const char *scheme) {
	if (!scheme) {
		return -1;
	}

	if (strlen(scheme) == 4 && memcmp("http", scheme, 4) == 0) {
		return 80;
	} else if (strlen(scheme) == 5 && memcmp("https", scheme, 5) == 0) {
		return 443;
	}

	return -1;
}
