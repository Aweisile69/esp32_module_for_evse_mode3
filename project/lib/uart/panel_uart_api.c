/*
 * EVCharger Panel Project
 * Copyright (c) 2025, WangQiWei, <3167914232@qq.com>
 */
#include "panel_uart_api.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief  串口接收数据暂存处理（单字节）
 * @param  value 串口收到的单字节数据
 * @return 无
 * @note   在串口中断服务函数中调用,将接收到的数据作为参数传入
 */
void uart_receive_input(uint8_t value)
{
    if(1 == rx_buf_out - rx_buf_in) 
    {
        //!!! 串口接收缓存已满，处理速度跟不上接收，需要考虑扩大rx_buffer
        // log_error("rx_buffer_full!!!!");
    }
    else if((rx_buf_in > rx_buf_out) && ((rx_buf_in - rx_buf_out) >= sizeof(uart_rx_buf))) 
    {
        //!!! 串口接收缓存已满，处理速度跟不上接收，需要考虑扩大rx_buffer
        // log_error("rx_buffer_full!!!!");
    }else 
    {
        //串口接收缓存未满
        //串口数据到达rx_buffer末尾，修改rx_buf_in至开头
        if(rx_buf_in >= (uint8_t *)(uart_rx_buf + sizeof(uart_rx_buf))) 
        {
            rx_buf_in = (uint8_t *)(uart_rx_buf);
        }
        *rx_buf_in ++ = value;
    }
}

/**
 * @brief  串口接收数据暂存处理（多字节）
 * @param  value 串口要接收的数据的源地址
 * @param  data_len 串口要接收的数据的数据长度
 * @return 无
 * @note   如需要支持一次多字节缓存，可调用该函数
 */
void uart_receive_buff_input(uint8_t value[], unsigned short data_len)
{
    unsigned short i = 0;
    for(i = 0; i < data_len; i++) 
    {
        uart_receive_input(value[i]);
    }
}

bool is_valid_function_num(uint8_t data) {

    return (data == 0x10)||(data == 0x11)||(data == 0x12)||(data == 0x14)||
    (data == 0x15)||(data == 0x18)||(data == 0x20)||(data == 0x30);
}

/**
 * @brief  串口数据处理服务
 * @param  无
 * @return 无
 * @note   在MCU主函数while循环中调用该函数，调用该函数时，不要加上任何判断条件
 */
void mcu_uart_service(void)
{
    static uint16_t process_buf_in = 0;
    uint8_t rx_value_len = 0;
    uint16_t offset = 0;
	uint8_t checksum = 0;
    
    while((process_buf_in < sizeof(uart_data_process_buf)) && with_data_rxbuff() > 0) 
    {
        uart_data_process_buf[process_buf_in ++] = take_byte_rxbuff();
    }

    if(process_buf_in < PROTOCOL_HEAD)
    return;
    
    while((process_buf_in - offset) >= PROTOCOL_HEAD)
    {
        if(uart_data_process_buf[offset + HEAD_FIRST] != FRAME_FIRST) 
        {
            offset ++;
            continue;
        }
        
        if(uart_data_process_buf[offset + HEAD_SECOND] != FRAME_SECOND) 
        {
            offset ++;
            continue;
        }  

        if(!is_valid_function_num(uart_data_process_buf[offset + FUNCTION_NUM])) 
        {
            offset ++;
            continue;
        }      

        rx_value_len = uart_data_process_buf[offset + LENGTH];
		
        if((process_buf_in - offset) < PROTOCOL_HEAD + rx_value_len + 1) 
        {
            break;
        }
		
		checksum = get_check_sum(   (uint8_t *)uart_data_process_buf + offset,
                                    PROTOCOL_HEAD + rx_value_len  );

        if(checksum != uart_data_process_buf[offset + PROTOCOL_HEAD + rx_value_len]) 
        {
            offset += rx_value_len + 1;
            continue;
        }
        data_handle(offset);

        offset += PROTOCOL_HEAD + rx_value_len + 1;
    }//end while

    process_buf_in -= offset;
    if(process_buf_in > 0) 
    {
        memcpy(  (char *)uart_data_process_buf, 
                    (const char *)uart_data_process_buf + offset, 
                    process_buf_in  );
    }
}

/**
 * @brief  串口协议初始化函数
 * @param  Null
 * @return Null
 * @note   在MCU初始化代码中调用该函数
 */
void mcu_uart_protocol_init(void)
{
    rx_buf_in = (unsigned char *)uart_rx_buf;
    rx_buf_out = (unsigned char *)uart_rx_buf;
}

void mcu_fnum_data_update(uint8_t fnum, uint8_t value[], uint8_t len)
{
    /* 添加功能码 */
    set_uart_frame_function_num(fnum);
    /* 添加数据 */
    write_uart_fram_data_buff(value, len);
    /* 添加所有数据帧通用的部分 */
    wifi_uart_write_frame(len);
}


