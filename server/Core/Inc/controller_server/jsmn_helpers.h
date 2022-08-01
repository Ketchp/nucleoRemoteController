/*
 * jsmn_helpers.h
 *
 *  Created on: Jul 10, 2022
 *      Author: stefan
 */

#ifndef INC_CONTROLLER_SERVER_JSMN_HELPERS_H_
#define INC_CONTROLLER_SERVER_JSMN_HELPERS_H_

#include "jsmn.h"
#include <stdint.h>

enum iterator_err
{
	JSMN_NOT_ITERABLE = UINT16_MAX
};

/**
 * Data structure for iterating over nested JSON document using tokens.
 */
typedef struct jsmn_iterator
{
	/**
	 * Either JSMN_OBJECT or JSMN_ARRAY.
	 */
	jsmntok_t *parent_token;

	/**
	 * Current state of iteration.
	 */
	jsmntok_t *current_token;

	/**
	 * Count of iterated top-level tokens.
	 */
	uint16_t idx;
} jsmn_iterator_t;


/**
 * Initializes iterator structure.
 * @param it Iterator structure.
 * @param json_value JSON value to iterate over.
 * @return Size of iterated object or JSMN_NOT_ITERABLE
 */
uint16_t init_iterator( jsmn_iterator_t *it, jsmntok_t *json_value );

/**
 * Iterates over objects in same depth.
 * @param it Iterator structure.
 * @return Next key token in object./ Next token in array.
 */
jsmntok_t *next_value( jsmn_iterator_t *it );

#endif /* INC_CONTROLLER_SERVER_JSMN_HELPERS_H_ */
