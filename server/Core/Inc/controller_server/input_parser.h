/*
 * input_parser.h
 *
 *  Created on: Jul 9, 2022
 *      Author: stefan
 */

#ifndef INC_CONTROLLER_SERVER_INPUT_PARSER_H_
#define INC_CONTROLLER_SERVER_INPUT_PARSER_H_

#include <stdint.h>

int16_t parse_msg( const char *msg, uint16_t msg_len );

enum msg_type
{
	MSG_INVALID,
	MSG_CMD_GET,
	MSG_CMD_SET,
	MSG_CMD_POLL
};

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

