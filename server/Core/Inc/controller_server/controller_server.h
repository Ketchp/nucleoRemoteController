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

#define MAX_CONNECTIONS 4
#define MEM_ERROR UINT16_MAX

enum value_type
{
	_int,
	_float,
	_string
};

enum connection_state
{
	C_UNUSED = 0,
	C_NEW = 1,
	C_PAGE_CHANGED = 2,
	C_CLOSING = 4
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


struct page
{
	const char *page_description;
	w_val_t *page_content;
	uint16_t widget_count;

	void (*update_callback)( uint16_t widget_id,
							 w_val_t *old_value );
};

struct connection
{
	uint16_t page_id;
	enum connection_state state;
	struct pbuf *send_queue;
};

struct ctrl_server
{
	struct page **pages;
	uint16_t page_count;
	uint8_t running;

	struct connection connections[ MAX_CONNECTIONS ];
	uint8_t currently_handled_connection;
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

err_t mainloop( void );


#endif /* CONTROLLER_SERVER_CONTROLLER_SERVER_H_ */
