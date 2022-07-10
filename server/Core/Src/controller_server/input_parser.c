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

extern struct ctrl_server server;

#define CONST_STR_LEN( x ) ( sizeof(( x )) / sizeof(( x )[ 0 ]) )

static const char ERR_RESPONSE_NOT_OBJECT[] = "{\"ERR\":\"Expected JSON object as message.\"}";
static const char ERR_RESPONSE_CMD_NOT_STRING[] = "{\"ERR\":\"CMD attribute must be an string.\"}";
static const char ERR_RESPONSE_UNKNOWN_CMD[] = "{ \"ERR\": \"Unknown CMD.\" }";
static const char ERR_RESPONSE_UNKNOWN_FIELD[] = "{\"ERR\":\"Unknown field, supported only CMD, VAL.\"}";
static const char ERR_RESPONSE_VAL_NOT_OBJECT[] = "{\"ERR\":\"Expected JSON object in VAL attribute.\"}";
static const char ERR_RESPONSE_VAL_WRG_RESOURCE[] = "{\"ERR\":\"Unsupported resource.\"}";
static const char ERR_RESPONSE_VAL_EMPTY[] = "{\"ERR\":\"Empty VAL attribute.\"}";
static const char ERR_RESPONSE_VAL_INVALID_PAGE_ID[] = "{\"ERR\":\"Invalid page ID.\"}";
static const char ERR_RESPONSE_VAL_NOT_EXPECTED[] = "{\"ERR\":\"VAL not expected with POLL command.\"}";
static const char ERR_RESPONSE_PAGE_OUT_OF_RANGE[] = "{\"ERR\":\"Page out of range.\"}";



static void parse_fields( const char *msg, jsmntok_t *tokens );
static uint16_t get_page( const char *msg, jsmntok_t *val_token );


enum parsing_status
{
	PARSE_CONTINUE,
	PARSE_ERR
};


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

	parse_fields( msg, tokens );

	mem_free( tokens );

	return parsed_tokens;
}


static void
parse_fields(
		const char *msg,
		jsmntok_t *tokens )
{
	connection_t *conn = server.currently_handled_connection;

	if( tokens[0].type != JSMN_OBJECT )
	{
		conn->response = ERR_RESPONSE_NOT_OBJECT;
		conn->response_len = CONST_STR_LEN( ERR_RESPONSE_NOT_OBJECT );
		conn->type = RESP_ERROR;
		return;
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
			conn->response_len = CONST_STR_LEN( ERR_RESPONSE_UNKNOWN_FIELD );
			conn->type = RESP_ERROR;
			return;
		}
	}

	if( cmd_token->type != JSMN_STRING )
	{
		conn->response = ERR_RESPONSE_CMD_NOT_STRING;
		conn->response_len = CONST_STR_LEN( ERR_RESPONSE_CMD_NOT_STRING );
		conn->type = RESP_ERROR;
		return;
	}

	uint16_t cmd_len = cmd_token->end - cmd_token->start;
	if( cmd_len == 4 && !memcmp( msg + cmd_token->start, "POLL", cmd_len ) )
	{
		if( val_token )
		{
			conn->response = ERR_RESPONSE_VAL_NOT_EXPECTED;
			conn->response_len = CONST_STR_LEN( ERR_RESPONSE_VAL_NOT_EXPECTED );
			conn->type = RESP_ERROR;
			return;
		}
		conn->type = RESP_VAL;
		return;
	}
	else if( cmd_len == 3 && !memcmp( msg + cmd_token->start, "GET", cmd_len ) )
	{
		uint16_t page_id = get_page( msg, val_token );
		if( page_id == ERR_PAGE_ID )
			return;

		if( page_id >= server.page_count )
		{
			conn->response = ERR_RESPONSE_PAGE_OUT_OF_RANGE;
			conn->response_len = CONST_STR_LEN( ERR_RESPONSE_PAGE_OUT_OF_RANGE );
			conn->type = RESP_ERROR;
			return;
		}

		conn->response = server.pages[ page_id ]->page_description;
		conn->response_len = server.pages[ page_id ]->page_desc_len;
		conn->type = RESP_PAGE;
		return;
	}
	else if( cmd_len == 3 && !memcmp( msg + cmd_token->start, "SET", cmd_len ) )
	{
		// TODO call callback's and update value
		conn->type = RESP_VAL;
		return;
	}
	else
	{
		conn->response = ERR_RESPONSE_UNKNOWN_CMD;
		conn->response_len = CONST_STR_LEN( ERR_RESPONSE_UNKNOWN_CMD );
		conn->type = RESP_ERROR;
		return;
	}

}


static uint16_t get_page( const char *msg, jsmntok_t *val_token )
{
	connection_t *conn = server.currently_handled_connection;
	if( val_token->type != JSMN_OBJECT )
	{
		conn->response = ERR_RESPONSE_VAL_NOT_OBJECT;
		conn->response_len = CONST_STR_LEN( ERR_RESPONSE_VAL_NOT_OBJECT );
		conn->type = RESP_ERROR;
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
				uintmax_t page_id = strtol( msg + page_id_token->start, &end, 10 );
				if( end > msg + page_id_token->start && page_id < UINT16_MAX && !errno )
					return page_id;
			}
			conn->response = ERR_RESPONSE_VAL_INVALID_PAGE_ID;
			conn->response_len = CONST_STR_LEN( ERR_RESPONSE_VAL_INVALID_PAGE_ID );
			conn->type = RESP_ERROR;
			return ERR_PAGE_ID;
		}
		else
		{
			conn->response = ERR_RESPONSE_VAL_WRG_RESOURCE;
			conn->response_len = CONST_STR_LEN( ERR_RESPONSE_VAL_WRG_RESOURCE );
			conn->type = RESP_ERROR;
			return ERR_PAGE_ID;
		}
	}
	conn->response = ERR_RESPONSE_VAL_EMPTY;
	conn->response_len = CONST_STR_LEN( ERR_RESPONSE_VAL_EMPTY );
	conn->type = RESP_ERROR;
	return ERR_PAGE_ID;
}

