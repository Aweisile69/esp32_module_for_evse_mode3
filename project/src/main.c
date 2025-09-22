#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "api_spiffs.h"



static const char *TAG = "main";
static char http_req_handler_buf[20480];

static esp_err_t handler_get_index_page(httpd_req_t *r);
static esp_err_t handler_get_favicon(httpd_req_t *r);
static esp_err_t handler_get_css(httpd_req_t *r);
static esp_err_t handler_get_js(httpd_req_t *r);

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "evse_mode3"
#define EXAMPLE_ESP_WIFI_PASS      "tospo123"
#define EXAMPLE_ESP_WIFI_CHANNEL   (5)
#define EXAMPLE_MAX_STA_CONN       (5)

static const httpd_uri_t get_index_page = 
{
    .uri        = "/",
    .method     = HTTP_GET,
    .handler    = handler_get_index_page,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_favicon = 
{
    .uri        = "/favicon.ico",
    .method     = HTTP_GET,
    .handler    = handler_get_favicon,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_css = 
{
    .uri        = "/css/style.css",
    .method     = HTTP_GET,
    .handler    = handler_get_css,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_js = 
{
    .uri        = "/js/script.js",
    .method     = HTTP_GET,
    .handler    = handler_get_js,
    .user_ctx   = NULL,
};

static const httpd_uri_t *http_uri_array[] = {
    &get_index_page,
    &get_favicon,
    &get_css,
    &get_js,
    NULL
};

static esp_err_t handler_get_index_page(httpd_req_t *r)
{
    char*  buf;         //request的缓存数据buffer
    size_t buf_len;     //request的缓存数据length
    int response;
    /* 解析请求 */
    /* 1.获取HOST字段的长度，分配空间，加入缓存 */
    buf_len = httpd_req_get_hdr_value_len(r, "Host") + 1;
    if (buf_len > 1) 
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(r, "Host", buf, buf_len) == ESP_OK) 
        {
            ESP_LOGI(TAG, "Requset -> Host: %s", buf);
        }
        free(buf);
    }

    /* 编码响应 */
    /* 发送html页面 */ 
    memset(http_req_handler_buf,0,sizeof(http_req_handler_buf));
    spiffs_readfile("/spiffs/index.html", 20480, http_req_handler_buf);
    response = httpd_send(r, http_req_handler_buf, strlen(http_req_handler_buf));

    return response;
}

static esp_err_t handler_get_favicon(httpd_req_t *r)
{
    char*  buf;         //request的缓存数据buffer
    size_t buf_len;     //request的缓存数据length
    int response;
    /* 解析请求 */
    /* 1.获取HOST字段的长度，分配空间，加入缓存 */
    buf_len = httpd_req_get_hdr_value_len(r, "Host") + 1;
    if (buf_len > 1) 
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(r, "Host", buf, buf_len) == ESP_OK) 
        {
            ESP_LOGI(TAG, "Requset -> Host: %s", buf);
        }
        free(buf);
    }

    /* 编码响应 */
    memset(http_req_handler_buf,0,sizeof(http_req_handler_buf));
    spiffs_readfile("/spiffs/favicon.ico", 20480, http_req_handler_buf);
    response = httpd_send(r, http_req_handler_buf, strlen(http_req_handler_buf));

    return response;
}

static esp_err_t handler_get_css(httpd_req_t *r)
{
    char*  buf;         //request的缓存数据buffer
    size_t buf_len;     //request的缓存数据length
    int response;
    /* 解析请求 */
    /* 1.获取HOST字段的长度，分配空间，加入缓存 */
    buf_len = httpd_req_get_hdr_value_len(r, "Host") + 1;
    if (buf_len > 1) 
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(r, "Host", buf, buf_len) == ESP_OK) 
        {
            ESP_LOGI(TAG, "Requset -> Host: %s", buf);
        }
        free(buf);
    }

    /* 编码响应 */
    memset(http_req_handler_buf,0,sizeof(http_req_handler_buf));
    spiffs_readfile("/spiffs/css/style.css", 20480, http_req_handler_buf);
    response = httpd_send(r, http_req_handler_buf, strlen(http_req_handler_buf));

    return response;
}

static esp_err_t handler_get_js(httpd_req_t *r)
{
    char*  buf;         //request的缓存数据buffer
    size_t buf_len;     //request的缓存数据length
    int response;
    /* 解析请求 */
    /* 1.获取HOST字段的长度，分配空间，加入缓存 */
    buf_len = httpd_req_get_hdr_value_len(r, "Host") + 1;
    if (buf_len > 1) 
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(r, "Host", buf, buf_len) == ESP_OK) 
        {
            ESP_LOGI(TAG, "Requset -> Host: %s", buf);
        }
        free(buf);
    }

    /* 编码响应 */
    memset(http_req_handler_buf,0,sizeof(http_req_handler_buf));
    spiffs_readfile("/spiffs/js/script.js", 20480, http_req_handler_buf);
    response = httpd_send(r, http_req_handler_buf, strlen(http_req_handler_buf));

    return response;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

/**
  * @brief  开启一个http服务器
  * @param  http_events_array 指向http uri的处理程序数组
  * @retval httpd_handle_t http服务器句柄，如果为NULL，代表http服务器启动失败
  * @note   使用默认的Http服务器配置，端口:80
  */
httpd_handle_t http_start_server(const httpd_uri_t *http_events_array[])
{
    ESP_LOGI(TAG, "Http Sever Start ......");
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* 使能-清除最少使用的缓存项，可以释放资源 */
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Http Server Port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) 
    {
        /* 注册HTTP事件 */
        ESP_LOGI(TAG, "Http Sever Start successfully!");
        ESP_LOGI(TAG, "Registering URI handlers ......");

        for (int i = 0; http_events_array[i] != NULL; i++)
        {
            httpd_register_uri_handler(server, http_events_array[i]);
        }
        return server;
    }
    ESP_LOGW(TAG, "Http Sever Start failed!");
    return NULL;
}

void app_main(void) 
{
    /* init spiffs */
    api_spiffs_init();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    http_start_server(http_uri_array);
}