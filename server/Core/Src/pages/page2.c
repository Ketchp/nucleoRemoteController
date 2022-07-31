/*
 * page2.c
 *
 *  Created on: Jul 30, 2022
 *      Author: stefan
 */

#include "page2.h"
#include <string.h>

const char *page2 = "{\"size\":[3,3],\"widgets\":["
					"{\"type\":\"button\", \"text\":\"previous page\"},"
					"{\"type\":\"label\", \"text\":\"Page 2\"},"
					"{\"type\":\"button\", \"text\":\"next page\"},"
					"{\"type\":\"entry\", \"text\":\"Login:\", \"hint\": \"user123\", \"position\":[1, 1]},"
					"{\"type\":\"label\", \"text\":\"Hint: admin\"},"
					"{\"type\":\"entry\", \"text\":\"Password:\", \"pass\": true, \"position\":[2, 1]},"
					"{\"type\":\"label\", \"text\":\"Hint admin\"}"
					"]}";


w_val_t values2[] = { { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _int,
						.enabled = 0 },
					  { .value.string_val = NULL,
						.val_type = _string,
						.enabled = 1 },
					  { .value.string_val = NULL,
						.val_type = _string,
						.enabled = 1 } };


void page2_callback( uint16_t widget_id, w_val_t *_ )
{
	assert( widget_id < 4 );
	assert( widget_id != 1 );

	if( widget_id == 0 && values2[ 0 ].value.int_val == 1 )
	{
		values2[ 0 ].value.int_val = 0;
		change_page( 1 );
	}

	// after password was entered
	if( widget_id == 3 )
	{
		// check if login and password is correct
		if( values2[ 2 ].value.string_val && !strcmp( values2[ 2 ].value.string_val, "admin")
		 && values2[ 3 ].value.string_val && !strcmp( values2[ 3 ].value.string_val, "admin") )
			change_page( 3 );

		// delete login and password
		mem_free( values2[ 2 ].value.string_val );
		mem_free( values2[ 3 ].value.string_val );
		values2[ 2 ].value.string_val = NULL;
		values2[ 3 ].value.string_val = NULL;
	}
}
