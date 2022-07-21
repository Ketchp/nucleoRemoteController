/*
 * controller_server.h
 *
 *  Created on: Jul 5, 2022
 *      Author: stefan
 */

#ifndef CONTROLLER_SERVER_CONTROLLER_SERVER_H_
#define CONTROLLER_SERVER_CONTROLLER_SERVER_H_

#include "lwip.h"
#include "lwip/tcp.h"
#include <stdint.h>

#define ERR_PAGE_ID UINT16_MAX
#define MAX_TOKEN_COUNT 256

enum value_type
{
	_int,
	_float,
	_string
};


typedef struct _widget_value
{
	union
	{
		int32_t int_val;
		float float_val;
		char *string_val;
	} value;
	enum value_type val_type;
	uint8_t enabled;
} w_val_t;


typedef struct page
{
	const char *page_description;
	uint16_t page_desc_len;

	w_val_t *page_content;
	uint16_t widget_count;

	void (*update_callback)( uint16_t widget_id,
							 w_val_t *old_value );
} page_t;


typedef enum connection_flags
{
	// connection waits for message
	C_IDLE = ( 1 ),

	// response has been queued for sending
	C_SENT = ( 1 << 1 ),

	/*
	 * IDLE and SENT are NOT mutually exclusive
	 * since message could be received but
	 * response has not been queued for sending
	 * due to memory shortage
	 */

	// response is allocated on heap and needs to be freed in sent callback
	C_ALLOCATED = ( 1 << 2 ),

	// callback after SET command called,
	// but allocating memory for PAGE response failed
	C_CALLBACK_CALLED = ( 1 << 3 ),

	// connection in process of closing
	C_CLOSING = ( 1 << 4 )
} connection_flag_t;

typedef struct connection
{
	uint16_t current_page_id;
	const char *response;
	uint16_t response_len;
	connection_flag_t flags;
} connection_t;

struct ctrl_server
{
	page_t **pages;
	uint16_t page_count;
	uint16_t initial_page;
	uint8_t running;

	connection_t *currently_handled_connection;
	w_val_t old_value;
	uint16_t widget_id;
	uint16_t requested_page;
	void (*idle_callback)();
};



void server_init( void );

uint16_t add_page(
		const char *page_description,
		w_val_t *page_content,
		uint16_t widget_count,
		void (*update_callback)( uint16_t widget_id,
								 w_val_t *old_value ) );

void register_idle_callback( void (*idle_callback)() );

void change_page( uint16_t page_id );

void set_start_page( uint16_t page_id );

err_t mainloop( void );


#endif /* CONTROLLER_SERVER_CONTROLLER_SERVER_H_ */

