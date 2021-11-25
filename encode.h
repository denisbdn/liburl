/* 
 * File:   encode.h
 * Author: bdn
 *
 * Created on 19 Февраль 2014 г., 18:50
 */

#ifndef ENCODE_H
#define	ENCODE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "uri_parse.h"

struct holder {
	struct mutable_path data;
	size_t size;
	struct uri uri_desc;
};

/**
 * функция проверят пустой объект или нет
 * для корректной проверки после создания память которую ханимает объект должна быть обнулена
 * а заполняться память должна только служебными функциями
 * @param h указатель на проверяемый объект
 * @return 1 если объект пустой и 0 в противном случае
 */
int is_empty_holder(const struct holder *h);

/**
 * функция чистит динамически проаллоцированные поля
 * @param h указатель на объект struct holder
 * @return возвращает исходный указатель
 */
struct holder *free_holder(struct holder *h);

/**
 * функция принимает кодирует uri
 * @param source структура которая описывает uri
 * @return @struct holder нужные динамические поля проаллоцированны
 */
struct holder encode(const struct uri *source);

/**
 * функция строит относительный урл
 * @param base база для урла
 * @param relative конкретный урл
 * @return @struct holder нужные динамические поля проаллоцированны
 */
struct holder relative_uri(const struct uri *base, const struct uri *relative);

/**
 * функция при нужде добавлет '\0' В конец строки
 * @param source описание урла
 * @return текстовую строку соответствующую описанию
 */
const char *to_string(struct holder *source);

/**
 * функция копирует делает глубокую копию @source в @dest
 * @param dest куда копировать
 * @param source откуда копировать
 */
void copy(struct holder *dest, const struct holder *source);

/**
 * функция возвращает дефолтный порт
 * @param scheme
 * @return -1 если заданная схема не найденна
 */
int defaultPort(const char *scheme);

#ifdef	__cplusplus
}
#endif

#endif	/* ENCODE_H */

