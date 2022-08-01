/*
 * input_parser.h
 *
 *  Created on: Jul 9, 2022
 *      Author: stefan
 */

#ifndef INC_CONTROLLER_SERVER_INPUT_PARSER_H_
#define INC_CONTROLLER_SERVER_INPUT_PARSER_H_

#include <stdint.h>

enum msg_type
{
	MSG_INVALID,
	MSG_CMD_GET,
	MSG_CMD_SET,
	MSG_CMD_POLL
};

/**
 * Parses message and stores results of parsing inside global ctrl-server structure.
 * - MSG_INVALID must set response message.
 * - MSG_CMD_GET must set requested page id.
 * - MSG_CMD_SET must set new and old value and widget id.
 * - MSG_CMD_POLL simply returns.
 * @return - JSMN_ERROR_NOMEM on insufficient memory for parsing or if over MAX_TOKEN_COUNT would be needed for parsing.
 * @return - enum msg_type for parsed message.
 */
int16_t parse_msg( const char *msg, uint16_t msg_len );


#endif /* INC_CONTROLLER_SERVER_INPUT_PARSER_H_ */

/*
 * Example communication
 *
 * --server established--
 * -> { "VERSION": 1, "PAGE": 0 }
 *
 * <- { "CMD": "GET", "VAL": { "PAGE": 0 } }
 * -> { "widgets": [ { "type": "button", "text": "Press Me!" } ] }
 *
 * <- { "CMD": "POLL" }
 * -> { "VAL": [ 1, 1 ] }
 *
 * <- { "CMD": "SET", "VAL": [ 0, 1 ] }
 * -> { "PAGE": 1 }
 *
 * <- { "CMD": "POLL" }
 * -> { "VAL": [ 1, 1, 0, 1 ] }
 *
 * <- { "CMD": "SET", "VAL": [ 1, 1 ] }
 * -> { "VAL": [ 1, 0, 1, 0 ] }
 */

