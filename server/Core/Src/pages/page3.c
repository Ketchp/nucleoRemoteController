/*
 * page3.c
 *
 *  Created on: Jul 31, 2022
 *      Author: stefan
 */

#include "page3.h"

const char *page3 = "{\"size\":[2,3],\"widgets\":["
					"{\"type\":\"button\", \"text\":\"previous page\"},"
					"{\"type\":\"label\", \"text\":\"Page 3\"},"
					"{\"type\":\"value\", \"value_type\": \"int32\", \"text\":\"Button\", \"special\": {\"off\": 0, \"on\": 1}, \"position\":[1, 0]},"
					"{\"type\":\"value\", \"value_type\": \"float\", \"text\":\"ADC1:\", \"unit\": \"V\"},"
					"{\"type\":\"value\", \"value_type\": \"float\", \"text\":\"ADC2:\", \"unit\": \"V\"}"
					"]}";


w_val_t values3[] = { { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _float,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _float,
						.enabled = 1 } };


void page3_callback( uint16_t widget_id, w_val_t *_ )
{
	assert( widget_id == 0 );

	if( widget_id == 0 && values3[ 0 ].value.int_val == 1 )
	{
		values3[ 0 ].value.int_val = 0;
		change_page( 2 );
	}
}
