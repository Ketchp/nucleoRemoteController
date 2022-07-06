/*
 * controller_server.c
 *
 *  Created on: Jul 5, 2022
 *      Author: stefan
 */

#include "controller_server.h"

#include <string.h>

struct ctrl_server server;
extern struct netif gnetif;
extern ip4_addr_t ipaddr;

static err_t _mainloop_init( void );
static err_t _mainloop( void );
static void _mainloop_deinit( void );

static err_t new_conn_callback( void *arg, struct tcp_pcb *new_pcb, err_t err );
static err_t recv_callback( void *arg, struct tcp_pcb *new_pcb, struct pbuf *buf, err_t err );
static void err_callback( void *arg, err_t err );


void server_init( void )
{
	// TODO
}

static inline uint16_t push_page( struct page *new_page )
{
	struct page **new_mem =
			(struct page **)mem_malloc( ( server.page_count + 1 ) * sizeof( *new_mem ) );
	if( !new_mem )
		return MEM_ERROR;

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
	struct page *new_page = (struct page *)mem_malloc( sizeof( *new_page ) );
	if( !new_page )
		return MEM_ERROR;

	uint16_t new_id = push_page( new_page );
	if( new_id == MEM_ERROR )
		mem_free( new_page );
	else
	{
		new_page->page_description = page_description;
		new_page->page_content = page_content;
		new_page->widget_count = widget_count;
		new_page->update_callback = update_callback;
	}
	return new_id;
}


void register_idle_callback( void (*idle_callback)() )
{
	server.idle_callback = idle_callback;
}

void change_page( uint16_t page_id )
{
	// TODO
}

void update_values( void )
{
	// TODO
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

	uint8_t free_id;

	for( free_id = 0; free_id < MAX_CONNECTIONS; ++free_id )
		if( server.connections[free_id].state == UNUSED )
			break;

	struct connection *conn;

	if( free_id == MAX_CONNECTIONS ||
		( conn = (struct connection *)mem_malloc( sizeof( *conn ) ) ) == NULL )
	{
		tcp_close( new_pcb );
		return ERR_MEM;
	}

	tcp_setprio( new_pcb, TCP_PRIO_MIN );

	conn->page_idx = 0;
	conn->state = NEW;

	tcp_arg( new_pcb, conn );

	tcp_recv( new_pcb, recv_callback );

	tcp_err( new_pcb, err_callback );

	return ERR_OK;
}


static err_t recv_callback( void *arg, struct tcp_pcb *new_pcb, struct pbuf *buf, err_t err )
{
	// TODO
	return ERR_OK;
}

static void err_callback( void *arg, err_t err )
{
	// TODO
}


