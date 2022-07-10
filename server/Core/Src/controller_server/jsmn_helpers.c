/*
 * jsmn_helpers.c
 *
 *  Created on: Jul 10, 2022
 *      Author: stefan
 */


#include "jsmn_helpers.h"

/**
 * @note Function has no way of checking end of JSON and can return invalid pointer after last JSON value.
 * @return Next token in same depth.
 */
static jsmntok_t *skip_token( jsmntok_t *current, uint16_t to_skip )
{
	while( to_skip )
	{
		if( current->type == JSMN_PRIMITIVE || current->type == JSMN_STRING )
		{
			// size always 1
			--to_skip;
			++current;
		}
		else if( current->type == JSMN_OBJECT )
		{
			to_skip += current->size * 2 - 1; // size * ( key, value ) pairs - ( current token )
			++current;
		}
		else if( current->type == JSMN_ARRAY )
		{
			to_skip += current->size - 1; // number of elements in array - ( current token )
			++current;
		}
		else
			// type JSMN_UNDEFINED or invalid
			break;
	}
	return current;
}

uint16_t init_iterator( jsmn_iterator_t *it, jsmntok_t *json_value )
{
	if( json_value->type != JSMN_OBJECT && json_value->type != JSMN_ARRAY )
		return JSMN_NOT_ITERABLE;

	it->parent_token = json_value;

	if( !json_value->size )
		it->current_token = NULL;
	else
		it->current_token = json_value + 1;

	it->idx = 0;
	return json_value->size;
}

jsmntok_t *next_value( jsmn_iterator_t *it )
{
	jsmntok_t *ret = it->current_token;

	// iterator is already past end
	if( it->parent_token->size <= it->idx )
		return ret;

	++it->idx;

	// iterator pointed to last element,
	// need to make it past end, and skipping token is pointless
	if( it->parent_token->size <= it->idx )
	{
		it->current_token = NULL;
		return ret;
	}


	if( it->parent_token->type == JSMN_ARRAY )
		it->current_token = skip_token( it->current_token, 1 );

	else if( it->parent_token->type == JSMN_OBJECT )
		it->current_token = skip_token( it->current_token, 2 );

	return ret;
}
