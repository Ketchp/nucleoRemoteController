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

/**
 * Structure for representing value of various widgets.
 */
typedef struct _widget_value
{
	/**
	 * Stores actual value in memory.
	 */
	union
	{
		int32_t int_val;
		float float_val;
		char *string_val;
	} value;

	/**
	 * Type of value.
	 */
	enum value_type val_type;

	/**
	 * Encodes whether widget is enabled.
	 */
	uint8_t enabled;
} w_val_t;


/**
 * Internal data structure for storing page data.
 */
typedef struct page
{
	/**
	 * Representation of page UI.
	 */
	const char *page_description;

	/**
	 * Length of page_description.
	 * @note Without trailing null.
	 */
	uint16_t page_desc_len;

	/**
	 * Widget values array.
	 */
	w_val_t *page_content;

	/**
	 * Length of values array.
	 */
	uint16_t widget_count;

	/**
	 * Callback called when GUI is changed.
	 */
	void (*update_callback)( uint16_t widget_id,
							 w_val_t *old_value );
} page_t;

/**
 * State of connection flags.
 */
typedef enum connection_flags
{
	/**
	 * Connection waits for message.
	 */
	C_IDLE = ( 1 ),

	/**
	 * Response has been queued for sending.
	 * IDLE and SENT are NOT mutually exclusive
	 * since message could be received but
	 * response has not been queued for sending
	 * due to memory shortage
	 */
	C_SENT = ( 1 << 1 ),

	/**
	 * response is allocated on heap and needs to be freed in sent callback
	 */
	C_ALLOCATED = ( 1 << 2 ),

	/**
	 * Callback after SET command called,
	 * but allocating memory for PAGE response failed.
	 */
	C_CALLBACK_CALLED = ( 1 << 3 ),

	/**
	 * Connection in process of closing.
	 */
	C_CLOSING = ( 1 << 4 )
} connection_flag_t;


/**
 * Structure representing single connection with client.
 */
typedef struct connection
{
	/**
	 * Id of page which is currently displayed by client.
	 */
	uint16_t current_page_id;

	/**
	 * Pointer to response message.
	 * This needs to stay intact until ACK is received.
	 */
	const char *response;

	/**
	 * Length of message being sent.
	 */
	uint16_t response_len;

	/**
	 * Connection state.
	 */
	connection_flag_t flags;
} connection_t;

/**
 * Structure for staring internal server data and pages.
 */
struct ctrl_server
{
	/**
	 * Array of pointers to pages.
	 * All pointers are reallocated when new page is added and space increased by one.
	 * This is algorithmically slow but potentially saves memory.
	 */
	page_t **pages;

	/**
	 * Number of registered pages.
	 */
	uint16_t page_count;

	/**
	 * Id of page which is first loaded when client connects.
	 */
	uint16_t initial_page;

	/**
	 * Server is up.
	 */
	uint8_t running;

	/**
	 * When calling API calls from callback this decides what
	 * connection should be modified.
	 */
	connection_t *currently_handled_connection;

	/**
	 * Temporary storage of old value on SET command.
	 */
	w_val_t old_value;

	/**
	 * Id of widget which got modified.
	 */
	uint16_t widget_id;

	/**
	 * Page requested by GET command.
	 */
	uint16_t requested_page;

	/**
	 * Callback called each processing cycle.
	 */
	void (*idle_callback)();
};


/**
 * Initializes server structure.
 * @note Does not start any communication yet.
 */
void server_init( void );


/**
 * Registers new page to controller.
 * @param page_description UI description of page.
 * @param page_content array of values for page widgets.
 * @param widget_count Count of value present widgets.
 * @param update_callback Callback called when value changes.
 * @return page_id of newly added page(integers from 0).
 */
uint16_t
add_page( const char *page_description,
		  w_val_t *page_content,
		  uint16_t widget_count,
		  void (*update_callback)( uint16_t widget_id,
								   w_val_t *old_value ) );


/**
 * Registers callback which will be called each processing loop from mainloop.
 * @param idle_callback Callback.
 */
void register_idle_callback( void (*idle_callback)( void ) );


/**
 * Changes displayed page.
 * @param page_id Id of new page.
 * @note Can only be called from change_value callback.
 */
void change_page( uint16_t page_id );


/**
 * Set initial page which will be shown as first to all new connections.
 * @param page_id New page id.
 */
void set_start_page( uint16_t page_id );


/**
 * Infinite communication loop.
 */
err_t mainloop( void );


#endif /* CONTROLLER_SERVER_CONTROLLER_SERVER_H_ */

