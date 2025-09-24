/*
 * EVCharger Panel Project
 * Copyright (c) 2025, WangQiWei, <3167914232@qq.com>
 */

/* include ------------------------------------------------------------------ */
#include "system.h"
#include "panel_uart_api.h"

#define DEFAULT_VALUE_RUNNING_INFO()    {           \        
    .charge_status  = EVSE_IDLE,                    \
    .power          = 3500.0f,                      \
    .voltage        = 220.0f,                       \
    .current        = 16.0f,                        \
    .net_status     = NET_STAT_DISCONNECTED,        \
}

/* 串口数据处理缓冲区 */
volatile uint8_t uart_data_process_buf[PROTOCOL_HEAD+UART_PROCESS_BUFF_LEN];
/* 串口接收数据缓冲区 */
volatile uint8_t uart_rx_buf[PROTOCOL_HEAD+UART_RX_BUFF_LEN];   
/* 串口发送数据缓冲区 */      
volatile uint8_t uart_tx_buf[PROTOCOL_HEAD+UART_TX_BUFF_LEN];        

volatile uint8_t *rx_buf_in;
volatile uint8_t *rx_buf_out;

volatile running_info_t g_running_info = DEFAULT_VALUE_RUNNING_INFO();

/* private function protypes -------------------------------------------------*/
static void _uart_write_data(uint8_t *in, unsigned short len);
static void _update_all(void *value);

/**
 * @brief  判断串口接收缓存中是否有数据
 * @param  Null
 * @return 是否有数据
 */
bool with_data_rxbuff(void)
{
    if(rx_buf_in != rx_buf_out)
        return true;
    else
        return false;
}

/**
 * @brief  读取队列1字节数据
 * @param  Null
 * @return Read the data
 */
uint8_t take_byte_rxbuff(void)
{
    uint8_t value = 0;
    
    if(rx_buf_out != rx_buf_in) {
        //有数据
        if(rx_buf_out >= (uint8_t *)(uart_rx_buf + sizeof(uart_rx_buf))) {
            //数据已经到末尾
            rx_buf_out = (uint8_t *)(uart_rx_buf);
        }
        
        value = *rx_buf_out ++; 
    }
    
    return value;
}

/**
 * @brief  向串口数据帧中的功能码写1字节数据
 * @param  function_num 功能码
 * @return 写入完成后的数据内容的总长度
 */
void set_uart_frame_function_num(uint8_t function_num)
{
    uart_tx_buf[FUNCTION_NUM] = function_num;
}

/**
 * @brief  向串口数据帧中的数据内容写1字节数据
 * @param  byte 写入字节值
 * @return 写入完成后的数据内容的总长度
 */
void write_uart_fram_data_byte(uint8_t byte)
{
    uint8_t *obj = (uint8_t *)uart_tx_buf + DATA_START;
    
    *obj = byte;
}

/**
 * @brief  向串口数据帧中的数据内容写多字节数据
 * @param  src  数据源地址
 * @param  len  数据长度
 * @return 写入完成后的数据内容的总长度
 */
void write_uart_fram_data_buff(uint8_t *src, unsigned short len)
{
    unsigned char *obj = (uint8_t *)uart_tx_buf + DATA_START;
    
    my_memcpy(obj,src,len);
}

/**
 * @brief  向主控板串口发送一帧数据
 * @param  len 数据帧中的数据内容长度
 * @return 其实也可以直接写在每个特定的发送函数中，(但这是通用操作，所以写成函数)
 */
void wifi_uart_write_frame(uint8_t len)
{
    /* 校验和，默认是0x00 */
    uint8_t check_sum = 0x00;
    
    /* 添加帧头 */
    uart_tx_buf[HEAD_FIRST] = FRAME_FIRST;
    uart_tx_buf[HEAD_SECOND] = FRAME_SECOND;

    /* 添加数据长度 */
    uart_tx_buf[LENGTH] = len;
    
    /* 计算校验和 */
    /* 需校验的数据长度 = 帧头(2字节)+功能码(1字节)+数据长度(1字节)+数据(len) */  
    len += PROTOCOL_HEAD;
    check_sum = get_check_sum((uint8_t *)uart_tx_buf, len);
    /* 添加校验和 */
    uart_tx_buf[len] = check_sum;
    
    /* 向主控板发送一帧数据 */
    _uart_write_data((uint8_t *)uart_tx_buf, len + 1);
}

/**
 * @brief  计算校验和
 * @param  pack 数据源指针
 * @param  pack_len 计算校验和长度
 * @return 校验和
 */
uint8_t get_check_sum(uint8_t pack[], uint16_t pack_len)
{
    uint16_t i;
    uint16_t check_sum = 0;
 
    for(i = 0; i < pack_len; i ++) {
        check_sum += *pack ++;
    }
    check_sum &= 0x00FF;// 舍弃进位部分, 只保留低8位
    return check_sum;
}

/**
 * @brief  内存拷贝
 * @param  dest 目标地址
 * @param  src 源地址
 * @param  count 拷贝数据个数
 * @return 数据处理完后的源地址
 */
void *my_memcpy(void *dest, const void *src, unsigned short count)  
{  
    unsigned char *pdest = (unsigned char *)dest;  
    const unsigned char *psrc  = (const unsigned char *)src;  
    unsigned short i;
    
    if(dest == NULL || src == NULL) { 
        return NULL;
    }
    
    if((pdest <= psrc) || (pdest > psrc + count)) {  
        for(i = 0; i < count; i ++) {  
            pdest[i] = psrc[i];  
        }  
    }else {
        for(i = count; i > 0; i --) {  
            pdest[i - 1] = psrc[i - 1];  
        }  
    }  
    
    return dest;  
}

/**
 * @brief  数据帧处理
 * @param  offset 数据起始位
 * @return Null
 */
void data_handle(unsigned short offset)
{
    /* 获取功能码 */
    uint8_t function_num = uart_data_process_buf[offset + FUNCTION_NUM];
    /* 获取value的起始地址 */
    uint8_t *data_start = (uint8_t *)&uart_data_process_buf[offset + DATA_START];
    /* 根据功能码选择对应的操作 */
    switch (function_num)
    {
        /* 更新实时运行参数-全部 */
        case FN_UPDT_RUN_INFO_ALL: 
        _update_all(data_start);
        break;

        default:
        //error
        break;
    }
}

/**
 * @brief  串口发送一段数据
 * @param[in] {in} 发送缓存指针
 * @param[in] {len} 数据发送长度
 * @return Null
 */
static void _uart_write_data(uint8_t *in, unsigned short len)
{
    if((NULL == in) || (0 == len)) {
        return;
    }
    
    while(len --) {
        uart_transmit_output(*in);
        in ++;
    }
}

/**
 * @brief   更新-充电状态
 * @param   vaule 接收到的数据内容起始地址
 * @return  无
 * @note    以下是测试序列：
 * 
 */
static void _update_all(void *value)
{
    //TODO 更新所有状态
    g_running_info = *((running_info_t *)value);
    return;
}
