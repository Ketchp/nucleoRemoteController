/*
 * page0.c
 *
 *  Created on: Jul 30, 2022
 *      Author: stefan
 */

#include "page0.h"

const char *page0 = "{\"size\":[2,3],\"widgets\":["
					"{\"type\":\"label\", \"text\":\"Page 0\", \"position\":[0, 1]},"
					"{\"type\":\"button\", \"text\":\"next page\"},"
					"{\"type\":\"button\", \"text\":\"Led 1\"},"
					"{\"type\":\"button\", \"text\":\"Led 2\"},"
					"{\"type\":\"button\", \"text\":\"Led 3\"}"
					"]}";


w_val_t values0[] = { { .value.int_val = 0,
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


void page0_callback( uint16_t widget_id, w_val_t *_ )
{
	assert( widget_id < 4 );

	if( widget_id == 0 && values0[ 0 ].value.int_val == 1 )
	{
		values0[ 0 ].value.int_val = 0;
		HAL_GPIO_WritePin( GPIOB, led_green_Pin, GPIO_PIN_RESET );
		HAL_GPIO_WritePin( GPIOB, led_blue_Pin, GPIO_PIN_RESET );
		HAL_GPIO_WritePin( GPIOB, led_red_Pin, GPIO_PIN_RESET );
		change_page( 1 );
	}
	if( widget_id == 1 && values0[ 1 ].value.int_val == 1 )
		HAL_GPIO_TogglePin( GPIOB, led_green_Pin );

	if( widget_id == 2 && values0[ 2 ].value.int_val == 1 )
		HAL_GPIO_TogglePin( GPIOB, led_blue_Pin );

	if( widget_id == 3 && values0[ 3 ].value.int_val == 1 )
		HAL_GPIO_TogglePin( GPIOB, led_red_Pin );
}
