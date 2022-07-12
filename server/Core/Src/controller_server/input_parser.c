/*
 * input_parser.c
 *
 *  Created on: Jul 9, 2022
 *      Author: stefan
 */

#include "input_parser.h"
#include "controller_server.h"
#include "jsmn_helpers.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

extern struct ctrl_server server;

static const char ERR_RESPONSE_NOT_OBJECT[] = "{\"ERR\":\"Expected JSON object as message.\"}";
static const char ERR_RESPONSE_CMD_NOT_STRING[] = "{\"ERR\":\"CMD attribute must be an string.\"}";
static const char ERR_RESPONSE_UNKNOWN_CMD[] = "{ \"ERR\": \"Unknown CMD.\" }";
static const char ERR_RESPONSE_UNKNOWN_FIELD[] = "{\"ERR\":\"Unknown field, supported only CMD, VAL.\"}";
static const char ERR_RESPONSE_VAL_NOT_OBJECT[] = "{\"ERR\":\"Expected JSON object as VAL attribute.\"}";
static const char ERR_RESPONSE_VAL_NOT_ARRAY[] = "{\"ERR\":\"Expected JSON array as VAL attribute.\"}";
static const char ERR_RESPONSE_VAL_WRONG_LEN[] = "{\"ERR\":\"Expected array as [ id, value ] pair inside VAL.\"}";
static const char ERR_RESPONSE_VAL_WRG_RESOURCE[] = "{\"ERR\":\"Unsupported resource.\"}";
static const char ERR_RESPONSE_VAL_EMPTY[] = "{\"ERR\":\"Empty VAL attribute.\"}";
static const char ERR_RESPONSE_VAL_INVALID_PAGE_ID[] = "{\"ERR\":\"Invalid page ID.\"}";
static const char ERR_RESPONSE_VAL_INVALID_WIDGET_ID[] = "{\"ERR\":\"Invalid widget ID.\"}";
static const char ERR_RESPONSE_VAL_NOT_EXPECTED[] = "{\"ERR\":\"VAL not expected with POLL command.\"}";
static const char ERR_RESPONSE_PAGE_OUT_OF_RANGE[] = "{\"ERR\":\"Page out of range.\"}";
static const char ERR_RESPONSE_VAL_WRONG_WIDGET_ID[] = "{\"ERR\":\"Expected integer as widget ID.\"}";
static const char ERR_RESPONSE_WIDGET_NOT_ENABLED[] = "{\"ERR\":\"Widget not enabled.\"}";
static const char ERR_RESPONSE_WRONG_VALUE_TYPE[] = "{\"ERR\":\"Wrong type for value.\"}";
static const char ERR_RESPONSE_CANT_PARSE_WIDGET_VALUE[] = "{\"ERR\":\"Error parsing widget value.\"}";



static uint16_t parse_fields( const char *msg, jsmntok_t *tokens );
static uint16_t get_page( const char *msg, jsmntok_t *val_token );
static uint8_t get_widget_val( const char *msg, jsmntok_t *val_token );



int16_t
parse_msg(
		const char *msg,
		uint16_t msg_len )
{
	jsmn_parser parser;
	jsmn_init( &parser );

	uint16_t token_count = 4;
	jsmntok_t *tokens = (jsmntok_t *)mem_malloc( sizeof( *tokens ) * token_count );

	int16_t parsed_tokens;
	while( tokens )
	{
		parsed_tokens = jsmn_parse( &parser, msg, msg_len, tokens, token_count );
		if( parsed_tokens != JSMN_ERROR_NOMEM )
			break;

		token_count *= 2;
		if( token_count > MAX_TOKEN_COUNT )
			break;

		jsmntok_t *new_t = realloc( tokens, sizeof( *tokens ) * token_count );
		if( !new_t )
			break;

		tokens = new_t;
	}

	if( !tokens )
		return JSMN_ERROR_NOMEM;

	if( parsed_tokens < 0 )
	{
		mem_free( tokens );
		return parsed_tokens;
	}

	int16_t msg_type = parse_fields( msg, tokens );

	mem_free( tokens );

	return msg_type;
}


static uint16_t
parse_fields(
		const char *msg,
		jsmntok_t *tokens )
{
	connection_t *conn = server.currently_handled_connection;

	if( tokens[0].type != JSMN_OBJECT )
	{
		conn->response = ERR_RESPONSE_NOT_OBJECT;
		conn->response_len = sizeof( ERR_RESPONSE_NOT_OBJECT ) - 1;
		return MSG_INVALID;
	}

	jsmntok_t *cmd_token = NULL;
	jsmntok_t *val_token = NULL;

	jsmn_iterator_t it;

	init_iterator( &it, tokens );

	jsmntok_t *current;

	while( ( current = next_value( &it ) ) != NULL )
	{
		uint16_t key_len = current->end - current->start;

		if( key_len == 3 && !memcmp( msg + current->start, "CMD", key_len ) )
			cmd_token = current + 1;

		else if( key_len == 3 && !memcmp( msg + current->start, "VAL", key_len ) )
			val_token = current + 1;

		else
		{
			conn->response = ERR_RESPONSE_UNKNOWN_FIELD;
			conn->response_len = sizeof( ERR_RESPONSE_UNKNOWN_FIELD ) - 1;
			return MSG_INVALID;
		}
	}

	if( cmd_token->type != JSMN_STRING )
	{
		conn->response = ERR_RESPONSE_CMD_NOT_STRING;
		conn->response_len = sizeof( ERR_RESPONSE_CMD_NOT_STRING ) - 1;
		return MSG_INVALID;
	}

	uint16_t cmd_len = cmd_token->end - cmd_token->start;
	if( cmd_len == 4 && !memcmp( msg + cmd_token->start, "POLL", cmd_len ) )
	{
		if( val_token )
		{
			conn->response = ERR_RESPONSE_VAL_NOT_EXPECTED;
			conn->response_len = sizeof( ERR_RESPONSE_VAL_NOT_EXPECTED ) - 1;
			return MSG_INVALID;
		}
		return MSG_CMD_POLL;
	}
	else if( cmd_len == 3 && !memcmp( msg + cmd_token->start, "GET", cmd_len ) )
	{
		uint16_t page_id = get_page( msg, val_token );
		if( page_id == ERR_PAGE_ID )
			return MSG_INVALID;

		if( page_id >= server.page_count )
		{
			conn->response = ERR_RESPONSE_PAGE_OUT_OF_RANGE;
			conn->response_len = sizeof( ERR_RESPONSE_PAGE_OUT_OF_RANGE ) - 1;
			return MSG_INVALID;
		}

		server.requested_page = page_id;
		return MSG_CMD_GET;
	}
	else if( cmd_len == 3 && !memcmp( msg + cmd_token->start, "SET", cmd_len ) )
	{
		if( !get_widget_val( msg, val_token ) )
			return MSG_INVALID;

		return MSG_CMD_SET;
	}
	else
	{
		conn->response = ERR_RESPONSE_UNKNOWN_CMD;
		conn->response_len = sizeof( ERR_RESPONSE_UNKNOWN_CMD ) - 1;
		return MSG_INVALID;
	}

}


static uint16_t get_page( const char *msg, jsmntok_t *val_token )
{
	connection_t *conn = server.currently_handled_connection;
	if( !val_token || val_token->type != JSMN_OBJECT )
	{
		conn->response = ERR_RESPONSE_VAL_NOT_OBJECT;
		conn->response_len = sizeof( ERR_RESPONSE_VAL_NOT_OBJECT ) - 1;
		return ERR_PAGE_ID;
	}

	jsmn_iterator_t it;

	init_iterator( &it, val_token );

	jsmntok_t *current;

	while( ( current = next_value( &it ) ) != NULL )
	{
		uint16_t key_len = current->end - current->start;

		if( key_len == 4 && !memcmp( msg + current->start, "PAGE", key_len ) )
		{
			jsmntok_t *page_id_token = current + 1;
			char first = msg[ page_id_token->start ];
			if( page_id_token->type == JSMN_PRIMITIVE && first >= '0' && first <= '9' )
			{
				errno = 0;
				char *end;
				long page_id = strtol( msg + page_id_token->start, &end, 10 );
				if( end > msg + page_id_token->start && page_id < UINT16_MAX && !errno )
					return page_id;
			}
			conn->response = ERR_RESPONSE_VAL_INVALID_PAGE_ID;
			conn->response_len = sizeof( ERR_RESPONSE_VAL_INVALID_PAGE_ID ) - 1;
			return ERR_PAGE_ID;
		}
		else
		{
			conn->response = ERR_RESPONSE_VAL_WRG_RESOURCE;
			conn->response_len = sizeof( ERR_RESPONSE_VAL_WRG_RESOURCE ) - 1;
			return ERR_PAGE_ID;
		}
	}
	conn->response = ERR_RESPONSE_VAL_EMPTY;
	conn->response_len = sizeof( ERR_RESPONSE_VAL_EMPTY ) - 1;
	return ERR_PAGE_ID;
}

/**
 * @return 1 on success, 0 on error
 */
static uint8_t get_widget_val( const char *msg, jsmntok_t *val_token )
{
	connection_t *conn = server.currently_handled_connection;

	if( !val_token || val_token->type != JSMN_ARRAY )
	{
		conn->response = ERR_RESPONSE_VAL_NOT_ARRAY;
		conn->response_len = sizeof( ERR_RESPONSE_VAL_NOT_ARRAY ) - 1;
		return 0;
	}

	if( val_token->size != 2 )
	{
		conn->response = ERR_RESPONSE_VAL_WRONG_LEN;
		conn->response_len = sizeof( ERR_RESPONSE_VAL_WRONG_LEN ) - 1;
		return 0;
	}

	jsmntok_t *id_token = val_token + 1;

	char first = msg[ id_token->start ];
	if( id_token->type != JSMN_PRIMITIVE || first < '0' || first > '9' )
	{
		conn->response = ERR_RESPONSE_VAL_WRONG_WIDGET_ID;
		conn->response_len = sizeof( ERR_RESPONSE_VAL_WRONG_WIDGET_ID ) - 1;
		return 0;
	}

	errno = 0;
	char *end;
	long widget_id = strtol( msg + id_token->start, &end, 10 );
	if( end == msg + id_token->start || widget_id >= UINT16_MAX || errno )
	{
		conn->response = ERR_RESPONSE_VAL_WRONG_WIDGET_ID;
		conn->response_len = sizeof( ERR_RESPONSE_VAL_WRONG_WIDGET_ID ) - 1;
		return 0;
	}

	page_t *current_page = server.pages[ conn->current_page_id ];

	if( current_page->widget_count <= widget_id )
	{
		conn->response = ERR_RESPONSE_VAL_INVALID_WIDGET_ID;
		conn->response_len = sizeof( ERR_RESPONSE_VAL_INVALID_WIDGET_ID ) - 1;
		return 0;
	}

	w_val_t *current_value = current_page->page_content + widget_id;

	server.widget_id = widget_id;
	memcpy( &server.old_value, current_value, sizeof( server.old_value ) );

	if( !current_value->enabled )
	{
		conn->response = ERR_RESPONSE_WIDGET_NOT_ENABLED;
		conn->response_len = sizeof( ERR_RESPONSE_WIDGET_NOT_ENABLED ) - 1;
		return 0;
	}

	jsmntok_t *w_val_token = id_token + 1;

	enum value_type received_type;
	if( w_val_token->type == JSMN_STRING )
		received_type = _string;
	else if( w_val_token->type == JSMN_PRIMITIVE )
	{
		char first = msg[ w_val_token->start ];
		if( first < '0' || first > '9' )
		{
			conn->response = ERR_RESPONSE_WRONG_VALUE_TYPE;
			conn->response_len = sizeof( ERR_RESPONSE_WRONG_VALUE_TYPE ) - 1;
			return 0;
		}
		received_type = _int;
		uint16_t msg_len = w_val_token->end - w_val_token->start;
		if( memchr( msg + w_val_token->start, '.', msg_len ) )
			received_type = _float;
	}
	else
	{
		conn->response = ERR_RESPONSE_WRONG_VALUE_TYPE;
		conn->response_len = sizeof( ERR_RESPONSE_WRONG_VALUE_TYPE ) - 1;
		return 0;
	}

	enum value_type target_type = current_value->val_type;

	if( target_type != received_type )
	{
		conn->response = ERR_RESPONSE_WRONG_VALUE_TYPE;
		conn->response_len = sizeof( ERR_RESPONSE_WRONG_VALUE_TYPE ) - 1;
		return 0;
	}


	errno = 0;
	if( target_type == _int )
	{
		long received_value = strtol( msg + w_val_token->start, &end, 10 );
		if( end > msg + w_val_token->start && received_value <= INT32_MAX && received_value >= INT32_MIN && !errno )
		{
			current_value->value.int_val = received_value;
			return 1;
		}
	}

	else if( target_type == _float )
	{
		float received_value = strtof( msg + w_val_token->start, &end );
		if( end > msg + w_val_token->start &&  !errno && isfinite( received_value ) )
		{
			current_value->value.float_val = received_value;
			return 1;
		}
	}
	else
	{
		assert( target_type == _string );
		uint16_t rec_len = w_val_token->end - w_val_token->start;
		char *new_str = mem_malloc( ( rec_len + 1 ) * sizeof( *new_str ) );
		if( new_str )
		{
			memcpy( new_str, msg + w_val_token->start, rec_len );
			new_str[ rec_len ] = '\0';
			current_value->value.string_val = new_str;
			return 1;
		}
	}


	conn->response = ERR_RESPONSE_CANT_PARSE_WIDGET_VALUE;
	conn->response_len = sizeof( ERR_RESPONSE_CANT_PARSE_WIDGET_VALUE ) - 1;
	return 0;
}

