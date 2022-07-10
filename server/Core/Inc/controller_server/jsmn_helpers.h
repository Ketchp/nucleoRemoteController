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

typedef struct jsmn_iterator
{
	jsmntok_t *parent_token;
	jsmntok_t *current_token;
	uint16_t idx;
} jsmn_iterator_t;


/**
 * @return Initializes iterator structure.
 */
uint16_t init_iterator( jsmn_iterator_t *it, jsmntok_t *json_value );

/**
 * Iterates over objects in same depth.
 * @return Next key token in object./ Next token in array.
 */
jsmntok_t *next_value( jsmn_iterator_t *it );

#endif /* INC_CONTROLLER_SERVER_JSMN_HELPERS_H_ */
