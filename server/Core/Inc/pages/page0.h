/*
 * page0.h
 *
 *  Created on: Jul 30, 2022
 *      Author: stefan
 */

#ifndef SRC_PAGES_PAGE0_H_
#define SRC_PAGES_PAGE0_H_

#include "controller_server.h"

extern const char *page0;

extern w_val_t values0[];

extern void page0_callback( uint16_t widget_id, w_val_t *_ );

#endif /* SRC_PAGES_PAGE0_H_ */
