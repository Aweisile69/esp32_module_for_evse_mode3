/* include ------------------------------------------------------------------ */
/* standard library --------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
/* firmware library --------------------------------------------------------- */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
/* others ------------------------------------------------------------------- */
#include "api_spiffs.h"

static const char *TAG = "api_spiffs.c";

void api_spiffs_init(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS ......");

    esp_vfs_spiffs_conf_t conf = 
    {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) 
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } 
        else if (ret == ESP_ERR_NOT_FOUND) 
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    ESP_LOGI(TAG, "Performing SPIFFS_check() .....");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } 
    else 
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }
}

bool spiffs_readfile(const char *path, uint16_t size, char *buffer)
{
    memset(buffer, 0, size);
    struct stat st;
    if (stat(path, &st))
    {
        ESP_LOGE(TAG, "Path:%snot found", path);
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (fread(buffer, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "fread failed");
        return false;
    }
    fclose(fp);
    return true;
}



