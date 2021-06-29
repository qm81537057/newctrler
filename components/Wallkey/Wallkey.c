#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "Wallkey.h"
#include "Led.h"
#include "Json_parse.h"
#include "Http.h"

#define UART1_TXD (UART_PIN_NO_CHANGE)
#define UART1_RXD (GPIO_NUM_5)
#define UART1_RTS (UART_PIN_NO_CHANGE)
#define UART1_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE 100
static const char *TAG = "WALLKEY";

uint8_t WallKeyCtl_Status;

void Wallkey_Init(void)
{
    //配置GPIO
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = 1 << UART1_RXD;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, UART1_TXD, UART1_RXD, UART1_RTS, UART1_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    WallKeyCtl_Status = WallKeyWait;
}

static int8_t Wallkey_Read(uint8_t *Key_Id, int8_t Switch)
{
    uint8_t data_u1[100];

    int len1 = uart_read_bytes(UART_NUM_1, data_u1, BUF_SIZE, 20 / portTICK_RATE_MS);
    if (len1 != 0) //读取到按键数据
    {
        len1 = 0;

        printf("data_u1=");
        for(int i=0;i<len1;i++)
        {
            printf("0x%02X ",data_u1[i]);
        }
        printf("\n");


        if ((data_u1[0] == 0x7e) && (data_u1[1] == 0x08) && (data_u1[2] == 0x0D) && (data_u1[12] == 0x0D)) //校验数据头尾
        {
            if ((data_u1[3] == Key_Id[0]) && (data_u1[4] == Key_Id[1]) && (data_u1[5] == Key_Id[2]) && (data_u1[6] == Key_Id[3])) //校验KEY ID是否满足
            {
                ESP_LOGI(TAG, "Key ID = %02x-%02x-%02x-%02x", data_u1[3], data_u1[4], data_u1[5], data_u1[6]); 
                ESP_LOGI(TAG, "Key channel = %02x-%02x", data_u1[7] ,data_u1[8]);
                ESP_LOGI(TAG, "Key value = %02x", data_u1[10]);

                return data_u1[10];

                //                  上按                      下按                      上释放                  下释放                  同时stop+上
                // if ((data_u1[10] == 0x01) || (data_u1[10] == 0x02) || (data_u1[10] == 0x0B) || (data_u1[10] == 0x0C) || (data_u1[10] == 0x04))
                // {
                //     return data_u1[10];
                // }
                // else
                // {
                //     ESP_LOGE(TAG, "Switch ERR");
                //     return -1;
                // }

                /*
                if (Switch == 0) //左边
                {
                    if ((data_u1[9] == KEY_DOU_LEFT_UP) || (data_u1[9] == KEY_DOU_LEFT_DOWN) || (data_u1[9] == KEY_RELEASE) || (data_u1[9] == KEY_SIN_DOU))
                    {
                        return data_u1[9];
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Switch Not Left");
                        return -1;
                    }
                }
                else if (Switch == 1) //右边
                {
                    if ((data_u1[9] == KEY_DOU_RIGHT_UP) || (data_u1[9] == KEY_DOU_RIGHT_DOWN) || (data_u1[9] == KEY_RELEASE) || (data_u1[9] == KEY_SIN_DOU))
                    {
                        return data_u1[9];
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Switch Not Right");
                        return -1;
                    }
                }
                else if (Switch == 2) //单控
                {
                    if ((data_u1[9] == KEY_SIN_UP) || (data_u1[9] == KEY_SIN_DOWN) || (data_u1[9] == KEY_RELEASE) || (data_u1[9] == KEY_SIN_DOU))
                    {
                        return data_u1[9];
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Switch Not SINGLE SWITCH");
                        return -1;
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "Switch Error");
                    return -1;
                }*/
            }
            else
            {
                ESP_LOGI(TAG, "Key Refuse,ID = %02x-%02x-%02x-%02x", data_u1[3], data_u1[4], data_u1[5], data_u1[6]);
                ESP_LOGI(TAG, "Key channel = %02x-%02x", data_u1[7] ,data_u1[8]);
                ESP_LOGI(TAG, "Key value = %02x", data_u1[10]);
                
                return -1;
            }
        }
    }
    return -1;
}

void Wallkey_App(uint8_t *Key_Id, int8_t Switch)
{

    int8_t key = 0;
    key = Wallkey_Read(Key_Id, Switch);

    if (key == 0x01)
    {
        if ((WallKeyCtl_Status == WallKeyUpStop) || (WallKeyCtl_Status == WallKeyDownStop) || (WallKeyCtl_Status == WallKeyWait))
        {
            WallKeyCtl_Status = WallKeyUpStart;
        }
        else if (WallKeyCtl_Status == WallKeyUpStart || WallKeyCtl_Status == WallKeyDownStart)
        {
            WallKeyCtl_Status = WallKeyUpStop;
        }
        printf("KEY_DOU_LEFT_UP\n");
    }
    
    else if (key == 0x02)
    {
        if ((WallKeyCtl_Status == WallKeyUpStop) || (WallKeyCtl_Status == WallKeyDownStop) || (WallKeyCtl_Status == WallKeyWait))
        {
            WallKeyCtl_Status = WallKeyDownStart;
        }
        else if (WallKeyCtl_Status == WallKeyUpStart || WallKeyCtl_Status == WallKeyDownStart)
        {
            WallKeyCtl_Status = WallKeyDownStop;
        }
        printf("KEY_DOU_LEFT_DOWN\n");
    }

    else if (key == 0x04)
    {
        work_status = WORK_AUTO;
        Led_Status = LED_STA_AUTO;
        // http_send_mes(POST_NORMAL);
    }

    /*if ((key == KEY_DOU_LEFT_UP) || (key == KEY_DOU_RIGHT_UP))
    {
        if ((WallKeyCtl_Status == WallKeyUpStop) || (WallKeyCtl_Status == WallKeyDownStop) || (WallKeyCtl_Status == WallKeyWait))
        {
            WallKeyCtl_Status = WallKeyUpStart;
        }
        else if (WallKeyCtl_Status == WallKeyUpStart || WallKeyCtl_Status == WallKeyDownStart)
        {
            WallKeyCtl_Status = WallKeyUpStop;
        }
        printf("KEY_DOU_LEFT_UP\n");
    }
    else if ((key == KEY_DOU_LEFT_DOWN) || (key == KEY_DOU_RIGHT_DOWN))
    {
        if ((WallKeyCtl_Status == WallKeyUpStop) || (WallKeyCtl_Status == WallKeyDownStop) || (WallKeyCtl_Status == WallKeyWait))
        {
            WallKeyCtl_Status = WallKeyDownStart;
        }
        else if (WallKeyCtl_Status == WallKeyUpStart || WallKeyCtl_Status == WallKeyDownStart)
        {
            WallKeyCtl_Status = WallKeyDownStop;
        }
        printf("KEY_DOU_LEFT_DOWN\n");
    }
    else if (key == KEY_SIN_DOU)
    {
        work_status = WORK_AUTO;
        Led_Status = LED_STA_AUTO;
        // http_send_mes(POST_NORMAL);
    }*/



}
