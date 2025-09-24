/*
 * EVCharger Panel Project
 * Copyright (c) 2025, WangQiWei, <3167914232@qq.com>
 */
 
#ifndef __MAIN_BOARD_API_H__
#define __MAIN_BOARD_API_H__

/* include ------------------------------------------------------------------ */
#include <stdint.h>
#include "system.h"
#include "protocol.h"

/* public function protypes ------------------------------------------------- */

/* APP interface */
void mcu_uart_protocol_init(void);
void mcu_uart_service(void);
void mcu_fnum_data_update(uint8_t fnum, uint8_t value[], uint8_t len);
/* Driver interface */
void uart_receive_buff_input(uint8_t value[], unsigned short data_len);
void uart_receive_input(uint8_t value);

#endif /* __MAIN_BOARD_API_H__ */
