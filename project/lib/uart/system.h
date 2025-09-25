/*
 * Common Uart Protocal Driver
 * Copyright (c) 2025, WangQiWei, <3167914232@qq.com>
 */
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

/* include ------------------------------------------------------------------ */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* 数据帧中各功能字节的位序 */
#define HEAD_FIRST                      0
#define HEAD_SECOND                     1        
#define FUNCTION_NUM                    2
#define LENGTH                          3
#define DATA_START                      4
/* 固定协议头的相关信息 */
#define PROTOCOL_HEAD                   0x04            // 固定协议头长度
#define FRAME_FIRST                     0xAA            // 帧头第一字节
#define FRAME_SECOND                    0x55            // 帧头第二字节
/* 功能码 */
#define FN_UPDT_RUN_INFO_ALL            0x10            // 更新实时运行参数-全部
#define FN_UPDT_PRAM_CONFIG             0x16            // 参数配置
#define FN_UPDT_RFID_CARD               0x17            // 卡片管理
#define FN_UPDT_ALARM_RECORD            0x18            // 告警记录

/**
 * @brief   充电桩状态(记录整个系统的状态)
 */
typedef enum{
    EVSE_REBOOT = 0,            // 刚启动
    EVSE_IDLE,                  // 空闲,未插枪&未刷卡
    EVSE_plugWaitSwipe,         // 等待刷卡,已插枪&未刷卡
    EVSE_swipeWaitPlug,         // 等待插枪,未插枪&已刷卡
    EVSE_swipePlugReady,        // 已插枪,已刷卡
    EVSE_CHARGING,              // 充电中
    EVSE_CHARGE_PAUSE,          // 充电暂停
    EVSE_CHARGE_STOP,           // 充电暂停
    EVSE_CHARGE_DONE,           // 充电完成
    EVSE_FAULT,                 // 充电故障
}evse_state_t;

/**
 * @brief   网络状态(是否连接到互联网)
 */
typedef enum{
    NET_STAT_DISCONNECTED = 0,      // 未连接(此时处于AP模式)
    NET_STAT_CONNECTED,             // 已连接(此时处于AP+STA模式)
}net_stat_t;

typedef struct running_info{
    uint8_t charge_status;          // 充电状态
    float power;                    // 功率
    float voltage;                  // 电压
    float current;                  // 电流
    uint8_t net_status;             // 网络状态
}running_info_t;

typedef struct param_config{
    float ov_threshold;             // 过压阈值
    float uv_threshold;             // 欠压阈值
    uint8_t leakagedc;              // dc漏电流阈值
    uint8_t leakageac;              // ac漏电流阈值
    uint8_t maxcc;                  // 最大充电电流
}param_config_t;

/* 串口数据缓冲区大小设置，如果RAM不够，可按需修改大小 */
#define UART_PROCESS_BUFF_LEN           32
#define UART_RX_BUFF_LEN                32
#define UART_TX_BUFF_LEN                32

/* 串口数据处理缓冲区 */
extern volatile uint8_t uart_data_process_buf[PROTOCOL_HEAD+UART_PROCESS_BUFF_LEN];
/* 串口接收数据缓冲区 */
extern volatile uint8_t uart_rx_buf[PROTOCOL_HEAD+UART_RX_BUFF_LEN];   
/* 串口发送数据缓冲区 */      
extern volatile uint8_t uart_tx_buf[PROTOCOL_HEAD+UART_TX_BUFF_LEN];        

extern volatile uint8_t *rx_buf_in;
extern volatile uint8_t *rx_buf_out;

extern volatile running_info_t g_running_info;
extern volatile param_config_t g_param_config;

/* public function protypes ------------------------------------------------- */

bool with_data_rxbuff(void);
uint8_t take_byte_rxbuff(void);
void set_uart_frame_function_num(uint8_t function_num);
void write_uart_fram_data_byte(uint8_t byte);
void write_uart_fram_data_buff(uint8_t *src, unsigned short len);
void wifi_uart_write_frame(uint8_t len);
uint8_t get_check_sum(uint8_t pack[], uint16_t pack_len);
void *my_memcpy(void *dest, const void *src, unsigned short count);
void data_handle(unsigned short offset);

#endif /* __SYSTEM_H__ */
