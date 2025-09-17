#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "api_spiffs.h"

static const char *TAG = "main";

void app_main(void) 
{
    /* init spiffs */
    api_spiffs_init();

    /* 测试下能否读取spiffs_image */
    static char image_array[4096];
    spiffs_readfile("/spiffs/index.html",4096,image_array);
    ESP_LOGI(TAG, "image:\n%s", image_array);
}