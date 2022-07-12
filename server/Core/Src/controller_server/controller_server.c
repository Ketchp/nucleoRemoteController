/*
 * controller_server.c
 *
 *  Created on: Jul 5, 2022
 *      Author: stefan
 */

#include "controller_server.h"
#include "input_parser.h"
#include "jsmn.h"

#include <string.h>
#include <stdio.h>

struct ctrl_server server;
extern struct netif gnetif;
extern ip4_addr_t ipaddr;

static err_t _mainloop_init( void );
static err_t _mainloop( void );
static void _mainloop_deinit( void );

static err_t new_conn_callback( void *arg, struct tcp_pcb *new_pcb, err_t err );
static err_t recv_callback( void *arg, struct tcp_pcb *new_pcb, struct pbuf *buf, err_t err );
static err_t poll_callback( void *arg, struct tcp_pcb *pcb );
static err_t sent_callback( void *arg, struct tcp_pcb *pcb, uint16_t len );
static void err_callback( void *arg, err_t err );

static void send_data( struct tcp_pcb *pcb, connection_t *conn );
static void close_server( struct tcp_pcb *pcb, connection_t *conn );

static char INIT_RESPONSE[] = "{\"VERSION\":1,\"PAGE\":     }"; // 5 blanks to hold up to UINT16_MAX page id's
static char ERR_RESPONSE_NOT_JSON[] = "{\"ERR\":\"Not valid JSON.\"}"; // 5 blanks to hold up to UINT16_MAX page id's

void server_init( void )
{
	if( server.pages )
		mem_free( server.pages );
	server.page_count = 0;
	server.currently_handled_connection = NULL;
	server.idle_callback = NULL;
	server.running = 0;
	server.initial_page = 0;
}

static void free_connection( connection_t *conn )
{
	if( conn->flags & C_ALLOCATED )
		mem_free( (void *)conn->response );
	mem_free( conn );
}

static inline uint16_t
push_page(
		struct page *new_page )
{
	page_t **new_mem = (page_t **)mem_malloc( ( server.page_count + 1 ) * sizeof( *new_mem ) );
	if( !new_mem )
		return ERR_PAGE_ID;

	memcpy( new_mem, server.pages, server.page_count * sizeof( *new_mem ) );
	new_mem[ server.page_count ] = new_page;

	return server.page_count++;
}

/**
 * Registers new page.
 * @return Unique page_id on successful creation, MEM_ERROR on memory error.
 */
uint16_t
add_page(
		const char *page_description,
		w_val_t *page_content,
		uint16_t widget_count,
		void (*update_callback)( uint16_t widget_id, w_val_t *old_value ) )
{
	page_t *new_page = (page_t *)mem_malloc( sizeof( *new_page ) );
	if( !new_page )
		return ERR_PAGE_ID;

	uint16_t new_id = push_page( new_page );
	if( new_id == ERR_PAGE_ID )
	{
		mem_free( new_page );
		return ERR_PAGE_ID;
	}

	new_page->page_description = page_description;
	new_page->page_desc_len = strlen( page_description );
	new_page->page_content = page_content;
	new_page->widget_count = widget_count;
	new_page->update_callback = update_callback;
	return new_id;
}

void set_start_page( uint16_t page_id )
{
#ifdef DEBUG
	assert( page_id < server.page_count );
#endif
	server.initial_page = page_id;
}

void
register_idle_callback(
		void (*idle_callback)() )
{
	server.idle_callback = idle_callback;
}

void
change_page(
		uint16_t page_id )
{
#ifdef DEBUG
	assert( "Can't call change_page outside of value change callback." == NULL );
#endif

	connection_t *conn = server.currently_handled_connection;

	conn->current_page_id = page_id;
}

err_t mainloop( void )
{
	err_t err;
	if( ( err = _mainloop_init() ) != ERR_OK )
		return err;

	err = _mainloop();

	_mainloop_deinit();

	return err;
}

static err_t
_mainloop_init( void )
{
	struct tcp_pcb *pcb = tcp_new();

	assert( pcb != NULL );

	err_t err = tcp_bind( pcb, &ipaddr, 9874 );

	if( err != ERR_OK )
	{
		memp_free( MEMP_TCP_PCB, pcb );
		return err;
	}

	struct tcp_pcb *listen_pcb = tcp_listen( pcb );

	if( !listen_pcb )
	{
		memp_free( MEMP_TCP_PCB, pcb );
		return ERR_MEM;
	}

	tcp_accept( listen_pcb, new_conn_callback );

	server.running = 1;

	return ERR_OK;
}

static void _mainloop_deinit( void )
{
	// TODO free pages
}


static err_t _mainloop( void )
{
	while( server.running )
	{
		ethernetif_input( &gnetif );
		sys_check_timeouts();

		if( server.idle_callback )
			server.idle_callback();
	}
	return ERR_OK;
}

static err_t
new_conn_callback(
		void *arg,
		struct tcp_pcb *new_pcb,
		err_t err )
{
	if( err != ERR_OK )
	{
		tcp_abort( new_pcb );
		return ERR_ABRT;
	}

	LWIP_UNUSED_ARG( arg );

	connection_t *conn = (connection_t *)mem_malloc( sizeof( *conn ) );
	char *resp = (char *)mem_malloc( sizeof( INIT_RESPONSE ) * sizeof( *resp ) );

	if( !conn || !resp )
	{
		mem_free( conn );
		mem_free( resp );
		tcp_abort( new_pcb );
		return ERR_ABRT;
	}

	strcpy( resp, INIT_RESPONSE );

	char buff[ 6 ];
	snprintf( buff, sizeof( buff ), "%5hu", server.initial_page );

	// TODO find way to get rid of hard-coded 20
	memcpy( resp + 20, buff, sizeof( buff ) - 1 );

	tcp_setprio( new_pcb, TCP_PRIO_MIN );

	conn->current_page_id = server.initial_page;
	conn->response = resp;
	conn->response_len = sizeof( INIT_RESPONSE ) - 1; // -1 for trailing '\0'
	conn->flags = C_ALLOCATED;

	tcp_arg( new_pcb, conn );

	tcp_recv( new_pcb, recv_callback );

	tcp_err( new_pcb, err_callback );

	// polling every ~2s
	tcp_poll( new_pcb, poll_callback, 4 );

	tcp_sent( new_pcb, sent_callback );

	send_data( new_pcb, conn );
	tcp_output( new_pcb ); // TODO find if output needs to be called from lwip callback

	return ERR_OK;
}



static err_t
recv_callback(
		void *arg,
		struct tcp_pcb *pcb,
		struct pbuf *msg_pbuf,
		err_t err )
{
	connection_t *conn = (connection_t *)arg;

	// connection closed by other side
	if( !msg_pbuf )
	{
		conn->flags |= C_CLOSING;

		/*
		 * C_IDLE | C_SENT
		 *    0        0    polling needs to recognize closing
		 *    0        1    can close immediately( no need to check ACK of last message )
		 *    1        0    can close immediately
		 *    1        1    unreachable
		 */

		if( conn->flags & ( C_IDLE | C_SENT ) )
			close_server( pcb, conn );

		return ERR_OK;
	}


	assert( conn->flags & C_IDLE ); // should not receive message before previous has been SENT
	// maybe should just return ERR_MEM to first deal with previous message

	if( err != ERR_OK )
	{
		// TODO do something smarter
		free_connection( conn );
		tcp_abort( pcb );
		pbuf_free( msg_pbuf );
		return ERR_ABRT;
	}

	server.currently_handled_connection = conn;

	assert( msg_pbuf->len == msg_pbuf->tot_len ); // TODO accept messages split in more pbufs

	int16_t msg_type = parse_msg( (char *)msg_pbuf->payload, msg_pbuf->len );

	// not enough memory to parse right now
	if( msg_type == JSMN_ERROR_NOMEM )
		return ERR_MEM;


	conn->flags &= ~C_IDLE;
	if( msg_type == JSMN_ERROR_INVAL )
	{
		conn->response = ERR_RESPONSE_NOT_JSON;
		conn->response_len = sizeof( ERR_RESPONSE_NOT_JSON ) - 1; // -1 for trailing '\0'
	}


	// if message is invalid the parse_msg is responsible for creating response
	// if( msg_type == MSG_INVALID );


	if( msg_type == MSG_CMD_GET )
	{
		page_t *req_page = server.pages[ server.requested_page ];
		conn->response = req_page->page_description;
		conn->response_len = req_page->page_desc_len;
	}

	if( msg_type == MSG_CMD_SET )
	{
		uint16_t page_id = conn->current_page_id;
		page_t *current_page = server.pages[ page_id ];
		if( current_page->update_callback )
		{
			current_page->update_callback( server.widget_id, &server.old_value );
		}

		if( server.old_value.val_type == _string )
			mem_free( server.old_value.value.string_val );

		if( page_id != conn->current_page_id )
		{
			// TODO send { "PAGE": X }
		}

		else
			msg_type = MSG_CMD_POLL; // just pretend we are POLLing
	}

	if( msg_type == MSG_CMD_POLL )
	{
		// send values
	}

	send_data( pcb, conn );

	tcp_recved( pcb, msg_pbuf->len );
	pbuf_free( msg_pbuf );

	return ERR_OK;
}

static void err_callback( void *arg, err_t err )
{
#ifdef DEBUG
	assert( arg != NULL );
#endif
	// error occurred so we just free connection
	connection_t *conn = (connection_t *)arg;

	free_connection( conn );
}


static err_t poll_callback( void *arg, struct tcp_pcb *pcb )
{
#ifdef DEBUG
	assert( arg != NULL );
#endif

	connection_t *conn = (connection_t *)arg;

	// message queue'd to send -> try sending data
	if( !( conn->flags & ( C_SENT | C_IDLE ) ) )
		send_data( pcb, conn );


	// server want's to close && no message to send or message already sent
	if( ( conn->flags & C_CLOSING ) && ( conn->flags & ( C_SENT | C_IDLE ) ) )
		close_server( pcb, conn );

	return ERR_OK;
}

static err_t sent_callback( void *arg, struct tcp_pcb *pcb, uint16_t len )
{
#ifdef DEBUG
	assert( arg != NULL );
#endif

	connection_t *conn = (connection_t *)arg;

#ifdef DEBUG
	assert( conn->response_len = len );
	assert( conn->flags & C_SENT );
	assert( !( conn->flags & C_IDLE ) );
#endif

	if( conn->flags & C_ALLOCATED )
	{
		conn->flags &= ~C_ALLOCATED;
		mem_free( (void *)conn->response );
	}

	conn->response = NULL;
	conn->flags |= C_IDLE;
	conn->flags &= ~C_SENT;

	return ERR_OK;
}

static void close_server( struct tcp_pcb *pcb, connection_t *conn )
{
	tcp_arg( pcb, NULL );
	tcp_recv( pcb, NULL );
	tcp_err( pcb, NULL );
	tcp_poll( pcb, NULL, 0 );

	err_t err = tcp_close( pcb );

	// successfully closed
	if( err == ERR_OK )
	{
		free_connection( conn );
		return;
	}

	// memory error while closing, we need to wait a little and try to close in poll
	tcp_arg( pcb, conn );
	tcp_err( pcb, err_callback );
	tcp_poll( pcb, poll_callback, 1 ); // more frequent polling this time ~0.5s
}


static void send_data( struct tcp_pcb *pcb, connection_t *conn )
{
	if( conn->response && conn->response_len <= tcp_sndbuf( pcb ) )
	{
		err_t err = tcp_write( pcb, conn->response, conn->response_len, 0 );
		if( err == ERR_OK )
			conn->flags |= C_SENT;
	}
}


