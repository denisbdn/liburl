#include <string.h>
#include <arpa/nameser_compat.h>
#include <stdio.h>

#include "uri_parse.h"

inline static int is_valid_ipv6(struct path host)
{
	if (!host.begin || !host.end)
		return 0;
	if (host.end <= host.begin)
		return 0;
	char prev = '\0';
	int colon = 0;
	int double_colon = 0;
	const char *str;
	for (str = host.begin; str < host.end; ++str) {
		switch (*str) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
				break;
			case ':':
				if (prev == ':')
					++double_colon;
				++colon;
				break;
			default:
				return 0;
		}
		prev = *str;
	}
	if (double_colon > 1)
		return 0;
	if (colon > 8)
		return 0;
	if (!double_colon && colon != 7)
		return 0;
	return 1;
}

inline static int is_valid_ipv4(struct path host)
{
	if (!host.begin || !host.end)
		return 0;
	if (host.end <= host.begin)
		return 0;
	if (*(host.begin) == '.')
		return 0;
	char prev = '\0';
	int dot = 0;
	const char *str;
	for (str = host.begin; str < host.end; ++str) {
		switch (*str) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				break;
			case '.':
				if (prev == '.')
					return 0;
				++dot;
				break;
			default:
				return 0;
		}
		prev = *str;
	}
	if (dot != 3)
		return 0;
	return 1;
}

/**
 * этот метод получает указатель на следующий за ':' символ
 * он должен уметь понимать что за символом - порт или это мусор
 * @param uri
 * @return
 *  0 если неверно разобран урл
 *  1 если строка заканчивается портом
 *  2 если строка заканчивается портом и потом /
 *  3 если строка заканчивается портом и потом ?
 *  4 если строка заканчивается портом и потом #
 */
inline static int is_port(const char *uri)
{
	enum state {
		NONE,
		NUMBER,
		
	} state = NONE;
	for (; *uri; ++uri) {
		switch (*uri) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				state = NUMBER;
				break;
			case '/':
				if (state == NUMBER)
					return 2;
				else
					return 0;
			case '?':
				if (state == NUMBER)
					return 3;
				else
					return 0;
			case '#':
				if (state == NUMBER)
					return 4;
				else
					return 0;
			default:
				return 0;
		}
	}
	if (state == NUMBER)
		return 1;
	else
		return 0;
}

// scheme://[[userinfo]@]foo.com[:port]]/[path][?query][#fragment] - сделанно
// [path][?query][#fragment] - сделанно
// имплементировать [] - группировка - она может быть в разных частях урла - сделанно (http://[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]:8080/)
// имплементировать IPv6 - сделанно
enum uri_res parse_uri(const char *uri, struct uri *build)
{
	if (!build || !uri)
		return BAD_PARAM;
	memset(build, 0, sizeof(*build));
	enum state {
		NONE,
		READ_AFTER_NONE,
		SCHEME_AFTER,
		READ_AFTER_SCHEME_AFTER,
		HOST_MUST,
		READ_AFTER_HOST_MUST,
		PATH_MUST,
		READ_AFTER_PATH_MUST,
		QUERY_MUST,
		READ_AFTER_QUERY_MUST,
		FRAGMENT_MUST,
		READ_AFTER_FRAGMENT_MUST,
	} state = NONE;
	const char *previos = uri;
	for (; *uri; ++uri) {
		const char *curr = uri;
		switch (*curr) {
			case ':': {
				int isport = 0;
				switch (state) {
					case READ_AFTER_NONE:
					if (!strncmp(uri, "://", strlen("://"))) {
						build->scheme.begin = previos;
						build->scheme.end = curr;
						previos = uri + 3;
						uri += (3 - 1);
						state = SCHEME_AFTER;
						break;
					}
					case READ_AFTER_SCHEME_AFTER:
					case READ_AFTER_HOST_MUST:
						if ((isport = is_port(++uri)) > 0) {
							switch (isport) {
								case 1:
									state = NONE; // break from the loop on the next step
									uri += strlen(uri);
									break;
								case 2:
									build->has_begin_path_slash = 1;
									state = PATH_MUST;
									uri = strchr(uri, '/');
									break;
								case 3:
									state = QUERY_MUST;
									uri = strchr(uri, '?');
									break;
								case 4:
									state = FRAGMENT_MUST;
									uri = strchr(uri, '#');
									break;
							}
							build->host.begin = previos;
							build->host.end = curr;
							build->port.begin = curr + 1;
							build->port.end = uri;
							if (isport != 1) {
								previos = uri + 1;
								uri += (1 - 1);
							} else {
								--uri;
							}
							break;
						} else {
							return BAD_PARSE;
						}
					case PATH_MUST:
						state = READ_AFTER_PATH_MUST;
					case READ_AFTER_PATH_MUST:
						break;
					case QUERY_MUST:
						state = READ_AFTER_QUERY_MUST;
					case READ_AFTER_QUERY_MUST:
						break;
					case FRAGMENT_MUST:
						state = READ_AFTER_FRAGMENT_MUST;
					case READ_AFTER_FRAGMENT_MUST:
						break;
					default:
						return BAD_PARSE;
				}
			}
			break;
			case '@': {
				switch (state) {
					case NONE:
						state = READ_AFTER_NONE;
					case READ_AFTER_NONE:
						break;
					case SCHEME_AFTER:
					case READ_AFTER_SCHEME_AFTER:
						build->userinfo.begin = previos;
						build->userinfo.end = curr;
						previos = uri + 1;
						uri += (1 - 1);
						state = HOST_MUST;
						break;
					case PATH_MUST:
						state = READ_AFTER_PATH_MUST;
					case READ_AFTER_PATH_MUST:
						break;
					case QUERY_MUST:
						state = READ_AFTER_QUERY_MUST;
					case READ_AFTER_QUERY_MUST:
						break;
					case FRAGMENT_MUST:
						state = READ_AFTER_FRAGMENT_MUST;
					case READ_AFTER_FRAGMENT_MUST:
						break;
					default:
						return BAD_PARSE;
				}
			}
			break;
			case '/': {
				switch (state) {
					case READ_AFTER_NONE:
//						build->host.begin = previos;
//						build->host.end = curr;
//						previos = uri + 1;
//						uri += (1 - 1);
						state = READ_AFTER_PATH_MUST;
						break;
					case READ_AFTER_HOST_MUST:
					case READ_AFTER_SCHEME_AFTER:
						build->host.begin = previos;
						build->host.end = curr;
					case NONE:
						build->has_begin_path_slash = 1;
						previos = uri + 1;
						uri += (1 - 1);
						state = PATH_MUST;
						break;
					case PATH_MUST:
						state = READ_AFTER_PATH_MUST;
					case READ_AFTER_PATH_MUST:
						break;
					case QUERY_MUST:
						state = READ_AFTER_QUERY_MUST;
					case READ_AFTER_QUERY_MUST:
						break;
					case FRAGMENT_MUST:
						state = READ_AFTER_FRAGMENT_MUST;
					case READ_AFTER_FRAGMENT_MUST:
						break;
					default:
						return BAD_PARSE;
				}
			}
			break;
			case '?': {
				switch (state) {
					case READ_AFTER_SCHEME_AFTER:
					case HOST_MUST:
					case READ_AFTER_HOST_MUST:
						build->host.begin = previos;
						build->host.end = curr;
						previos = uri + 1;
						uri += (1 - 1);
						state = QUERY_MUST;
						break;
					case READ_AFTER_NONE:
					case PATH_MUST:
					case READ_AFTER_PATH_MUST:
						build->path.begin = previos;
						build->path.end = curr;
					case NONE:
						previos = uri + 1;
						uri += (1 - 1);
						state = QUERY_MUST;
						break;
					case QUERY_MUST:
					case READ_AFTER_QUERY_MUST:
					case FRAGMENT_MUST:
					case READ_AFTER_FRAGMENT_MUST:
						break;
					default:
						return BAD_PARSE;
				}
			}
			break;
			case '#': {
				switch (state) {
					case READ_AFTER_SCHEME_AFTER:
					case HOST_MUST:
					case READ_AFTER_HOST_MUST:
						build->host.begin = previos;
						build->host.end = curr;
						previos = uri + 1;
						uri += (1 - 1);
						state = FRAGMENT_MUST;
						break;
					case READ_AFTER_NONE:
					case PATH_MUST:
					case READ_AFTER_PATH_MUST:
						build->path.begin = previos;
						build->path.end = curr;
					case NONE:
						previos = uri + 1;
						uri += (1 - 1);
						state = FRAGMENT_MUST;
						break;
					case QUERY_MUST:
					case READ_AFTER_QUERY_MUST:
						build->query.begin = previos;
						build->query.end = curr;
						previos = uri + 1;
						uri += (1 - 1);
						state = FRAGMENT_MUST;
						break;
					//case FRAGMENT_MUST:
					//	state = READ_AFTER_FRAGMENT_MUST;
					//case READ_AFTER_FRAGMENT_MUST:
					//	break;
					default:
						return BAD_PARSE;
				}
			}
			break;
			case '[': {
				const char *end_group;
				if (!(end_group = strchr(uri, ']')))
					return BAD_PARSE;
				// проверяем возможно это ipv6 адрес
				struct path host = {uri + 1, end_group};
				if (is_valid_ipv6(host)) {
					switch (state) {
						case NONE:
						case READ_AFTER_NONE:
						case SCHEME_AFTER:
						case READ_AFTER_SCHEME_AFTER:
						case HOST_MUST:
							state =	READ_AFTER_HOST_MUST;
							previos = uri;
							build->host_type = HOST_TYPE_IPv6;
							break;
						default:
							break;
					}
				}
				uri = end_group;
			}
			// дальше анализируем как будто прочитали любой символ
			default:
				switch (state) {
					case NONE:
						state = READ_AFTER_NONE;
						break;
					case SCHEME_AFTER:
						state = READ_AFTER_SCHEME_AFTER;
						break;
					case HOST_MUST:
						state = READ_AFTER_HOST_MUST;
						break;
					case PATH_MUST:
						state = READ_AFTER_PATH_MUST;
						break;
					case QUERY_MUST:
						state = READ_AFTER_QUERY_MUST;
						break;
					case FRAGMENT_MUST:
						state = READ_AFTER_FRAGMENT_MUST;
						break;
					default:
						break;
				}
		}
	}
	switch (state) {
		case NONE:
			break;
		case SCHEME_AFTER:
		case HOST_MUST:
			return BAD_PARSE;
		case READ_AFTER_SCHEME_AFTER:
		case READ_AFTER_HOST_MUST:
			build->host.begin = previos;
			build->host.end = uri;
			break;
		case READ_AFTER_NONE:
//			build->host.begin = previos;
//			build->host.end = uri;
//			break;
		case PATH_MUST:
		case READ_AFTER_PATH_MUST:
			build->path.begin = previos;
			build->path.end = uri;
			break;
		case QUERY_MUST:
		case READ_AFTER_QUERY_MUST:
			build->query.begin = previos;
			build->query.end = uri;
			break;
		case FRAGMENT_MUST:
		case READ_AFTER_FRAGMENT_MUST:
			build->fragment.begin = previos;
			build->fragment.end = uri;
			break;
		default:
			return BAD_PARSE;
	}
	if (build->path.begin && build->path.end && build->path.end != build->path.begin && *(build->path.end - 1) == '/')
		build->has_end_path_slash = 1;
	if (build->host_type != HOST_TYPE_IPv6) {
		if (is_valid_ipv4(build->host))
			build->host_type = HOST_TYPE_IPv4;
	}
	return OK;
}

char *build_string_from_path(struct path p, char *str, size_t *size)
{
	if (!p.begin || !p.end || !size)
		return NULL;
	char format[256];
	sprintf(format, "%%.%zus", p.end - p.begin);
	int res = snprintf(str, *size, format, p.begin);
	*size = res;
	return str;
}

const char *build_static_string_from_path(const struct path p)
{
	static char arr[4096];
	size_t size = sizeof(arr);
	return (const char *)build_string_from_path(p, arr, &size);
}