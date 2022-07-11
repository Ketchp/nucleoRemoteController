/*
 * controller_server.c
 *
 *  Created on: Jul 5, 2022
 *      Author: stefan
 */

#include "controller_server.h"
#include "input_parser.h"

#include <string.h>

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

static err_t start_sending( struct tcp_pcb *pcb, connection_t *conn );


void server_init( void )
{
	server.currently_handled_connection = NULL;
}

static inline uint16_t push_page( struct page *new_page )
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
uint16_t add_page(
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


void register_idle_callback( void (*idle_callback)() )
{
	server.idle_callback = idle_callback;
}

void change_page( uint16_t page_id )
{
	if( server.currently_handled_connection == NULL )
		return;

	connection_t *conn = server.currently_handled_connection;

	conn->current_page = server.pages[ page_id ];
	conn->state |= C_PAGE_CHANGED;
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

static err_t _mainloop_init( void )
{
	struct tcp_pcb *pcb = tcp_new();

	err_t err = tcp_bind( pcb, &ipaddr, 9874 );

	if( err != ERR_OK )
	{
		memp_free( MEMP_TCP_PCB, pcb );
		return err;
	}

	pcb = tcp_listen( pcb );

	if( !pcb )
	{
		memp_free( MEMP_TCP_PCB, pcb );
		return ERR_MEM;
	}

	tcp_accept( pcb, new_conn_callback );


	// TODO initialize server
	server.running = 1;

	return ERR_OK;
}

static void _mainloop_deinit( void )
{

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

static err_t new_conn_callback( void *arg, struct tcp_pcb *new_pcb, err_t err )
{
	if( err != ERR_OK )
	{
		tcp_abort( new_pcb );
		return ERR_ABRT;
	}

	LWIP_UNUSED_ARG( arg );

	connection_t *conn = (connection_t *)mem_malloc( sizeof( *conn ) );

	if( !conn )
	{
		tcp_abort( new_pcb );
		return ERR_ABRT;
	}

	tcp_setprio( new_pcb, TCP_PRIO_MIN );

	conn->current_page = server.pages[ 0 ];
	conn->state = C_NEW;
	conn->response = NULL;
	conn->response_len = 0;

	tcp_arg( new_pcb, conn );

	tcp_recv( new_pcb, recv_callback );

	tcp_err( new_pcb, err_callback );

	tcp_poll( new_pcb, poll_callback, 6 );

	tcp_sent( new_pcb, sent_callback );

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
	if( !msg_pbuf )
	{
		tcp_arg( pcb, NULL );
		tcp_sent( pcb, NULL );
		tcp_recv( pcb, NULL );
		tcp_err( pcb, NULL );
		conn->state = C_UNUSED;
		mem_free( conn );
		tcp_close( pcb );
		return ERR_OK;
	}
	if( err != ERR_OK )
	{
		if( err == ERR_ABRT )
			pbuf_free( msg_pbuf );
		return err;
	}

	server.currently_handled_connection = conn;

	assert( msg_pbuf->len == msg_pbuf->tot_len );
	int16_t parse_err = parse_msg( (char *)msg_pbuf->payload, msg_pbuf->len );

	char *load = (char *)mem_malloc( msg_pbuf->len );
	if( !load )
		return ERR_MEM;

	memcpy( load, msg_pbuf->payload, msg_pbuf->len );
	conn->response = load;
	conn->response_len = msg_pbuf->len;

	err_t w_err = start_sending( pcb, conn );

	if( w_err == ERR_OK )
	{
		pbuf_free( msg_pbuf );
		tcp_recved( pcb, conn->response_len );
	}

	return w_err;
}

static void err_callback( void *arg, err_t err )
{
	// TODO
}

static err_t start_sending( struct tcp_pcb *pcb, connection_t *conn )
{
	if( conn->response && conn->response_len <= tcp_sndbuf( pcb ) )
	{
		err_t err = tcp_write( pcb,
				conn->response,
				conn->response_len,
				0 );

		if( err != ERR_OK )
			return err;
	}
	return ERR_OK;
}

static err_t poll_callback( void *arg, struct tcp_pcb *pcb )
{
	err_t err = start_sending( pcb, (connection_t *)arg );
	tcp_output( pcb );
	return err;
}

static err_t sent_callback( void *arg, struct tcp_pcb *pcb, uint16_t len )
{
	connection_t *conn = (connection_t *)arg;
	mem_free( (void *)conn->response );
	conn->response = NULL;
	conn->response_len = 0;
	return ERR_OK;
}

static err_t close_server( struct tcp_pcb *pcb, connection_t *conn )
{
	return ERR_OK;
}

