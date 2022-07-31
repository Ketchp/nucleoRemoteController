/*
 * page1.c
 *
 *  Created on: Jul 30, 2022
 *      Author: stefan
 */

#include "page1.h"

const char *page1 = "{\"size\":[2,3],\"widgets\":["
					"{\"type\":\"button\", \"text\":\"previous page\"},"
					"{\"type\":\"label\", \"text\":\"Page 1\"},"
					"{\"type\":\"button\", \"text\":\"next page\"},"
					"{\"type\":\"label\", \"text\":\"Switches:\"},"
					"{\"type\":\"switch\", \"text\":\"Led 1,Led 2,Led 3\", \"show_zero\": false},"
					"{\"type\":\"switch\", \"text\":\"Off,Led 1,Led 2,Led 3\", \"vertical\": true}"
					"]}";


w_val_t values1[] = { { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 },
					  { .value.int_val = 0,
						.val_type = _int,
						.enabled = 1 } };


void page1_callback( uint16_t widget_id, w_val_t *_ )
{
	assert( widget_id < 4 );

	if( widget_id == 0 && values1[ 0 ].value.int_val == 1 )
	{
		values1[ 0 ].value.int_val = 0;
		values1[ 2 ].value.int_val = 0;
		values1[ 3 ].value.int_val = 0;
		change_page( 0 );
	}

	if( widget_id == 1 && values1[ 1 ].value.int_val == 1 )
	{
		values1[ 1 ].value.int_val = 0;
		change_page( 2 );
	}

	if( widget_id == 2 )
	{
		values1[ 3 ].value.int_val = values1[ 2 ].value.int_val;
	}

	HAL_GPIO_WritePin( GPIOB, led_green_Pin, GPIO_PIN_RESET );
	HAL_GPIO_WritePin( GPIOB, led_blue_Pin, GPIO_PIN_RESET );
	HAL_GPIO_WritePin( GPIOB, led_red_Pin, GPIO_PIN_RESET );
	switch( values1[ 3 ].value.int_val )
	{
	case 1:
		HAL_GPIO_WritePin( GPIOB, led_green_Pin, GPIO_PIN_SET );
		break;
	case 2:
		HAL_GPIO_WritePin( GPIOB, led_blue_Pin, GPIO_PIN_SET );
		break;
	case 3:
		HAL_GPIO_WritePin( GPIOB, led_red_Pin, GPIO_PIN_SET );
	}
}
