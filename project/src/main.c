#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "api_spiffs.h"

static const char *TAG = "main";

static char image_array[20480];
void app_main(void) 
{
    /* init spiffs */
    api_spiffs_init();

    spiffs_readfile("/spiffs/index.html",20480,image_array);
    ESP_LOGI(TAG, "image:\n%s", image_array);
}