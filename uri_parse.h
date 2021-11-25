/* 
 * File:   uri_parse.h
 * Author: bdn
 *
 * Created on 28 Январь 2014 г., 10:30
 */

#ifndef URI_PARSE_H
#define	URI_PARSE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <string.h>
	
	struct mutable_path {
		char *begin;
		char *end;
	};

	struct path {
		const char *begin;
		const char *end;
	};

	enum HOST_TYPE {
		HOST_TYPE_DOMAIN = 0,
		HOST_TYPE_IPv4,
		HOST_TYPE_IPv6
	};

	struct uri {
		struct path scheme;
		struct path userinfo;
		struct path host;
		struct path port;
		struct path path;
		struct path query;
		struct path fragment;
		int has_end_path_slash;
		int has_begin_path_slash;
		enum HOST_TYPE host_type;
	};

	enum uri_res {
		OK,
		BAD_PARAM,
		BAD_PARSE,
	};

	/**
	 * функция парсит uri на элементы
     * @param uri входная строка - в UTF-8
     * @param build результат парсинга - осторожно ссылается на переданную строку
     * @return результат парсинга
     */
	enum uri_res parse_uri(const char *uri, struct uri *build);

	/**
	 * функция копирует часть struct uri в строку
     * @param p часть struct uri которую надо копировать
     * @param str указатель на буфер куда копировать
     * @param size указатель на размер буфера (при входе), и колличество необходимых байтов (при выходе)
     * @return 
     */
	char *build_string_from_path(struct path p, char *str, size_t *size);

	/**
	 * функция копирует часть struct uri в статический буфер
     * @param p часть struct uri которую надо копировать
     * @return указетль на статический буфер куда копировали результат
     */
	const char *build_static_string_from_path(const struct path p);

#define ZERROSET(a) memset(&(a), 0, sizeof((a)))

#ifdef	__cplusplus
}
#endif

#endif	/* URI_PARSE_H */

