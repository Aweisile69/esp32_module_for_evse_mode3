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
#include "cJSON.h"

#include "api_spiffs.h"
#include "system.h"



static const char *TAG = "main";
static char http_req_handler_buf[20480];

static esp_err_t handler_ping(httpd_req_t *r);
static esp_err_t handler_get_index_page(httpd_req_t *r);
static esp_err_t handler_get_favicon(httpd_req_t *r);
static esp_err_t handler_get_css(httpd_req_t *r);
static esp_err_t handler_get_js(httpd_req_t *r);
static esp_err_t handler_get_api_status(httpd_req_t *r);
static esp_err_t handler_get_api_config(httpd_req_t *r);
static esp_err_t handler_post_api_config(httpd_req_t *r);
static esp_err_t handler_api_cards_get(httpd_req_t *r);
static esp_err_t handler_api_cards_post(httpd_req_t *r);
static esp_err_t handler_api_cards_delete(httpd_req_t *r);
static esp_err_t handler_api_alarms_get(httpd_req_t *r);
static esp_err_t handler_api_alarms_delete(httpd_req_t *r);

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

static const httpd_uri_t get_api_ping = {
    .uri        = "/api/ping",
    .method     = HTTP_GET,
    .handler    = handler_ping,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_status = {
    .uri        = "/api/status",
    .method     = HTTP_GET,
    .handler    = handler_get_api_status,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_config_get = {
    .uri        = "/api/config",
    .method     = HTTP_GET,
    .handler    = handler_get_api_config,
    .user_ctx   = NULL,
};

static const httpd_uri_t post_api_config_post = {
    .uri        = "/api/config",
    .method     = HTTP_POST,
    .handler    = handler_post_api_config,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_cards_get = {
    .uri        = "/api/cards",
    .method     = HTTP_GET,
    .handler    = handler_api_cards_get,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_cards_post = {
    .uri        = "/api/cards",
    .method     = HTTP_POST,
    .handler    = handler_api_cards_post,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_cards_delete = {
    .uri        = "/api/cards/*",  // 通配符匹配 /api/cards/{id}
    .method     = HTTP_DELETE,
    .handler    = handler_api_cards_delete,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_alarms_get = {
    .uri        = "/api/alarms",
    .method     = HTTP_GET,
    .handler    = handler_api_alarms_get,
    .user_ctx   = NULL,
};

static const httpd_uri_t get_api_alarms_delete = {
    .uri        = "/api/alarms",
    .method     = HTTP_DELETE,
    .handler    = handler_api_alarms_delete,
    .user_ctx   = NULL,
};

static const httpd_uri_t *http_uri_array[] = {
    &get_index_page,
    &get_favicon,
    &get_css,
    &get_js,
    &get_api_ping,
    &get_api_status,
    &get_api_config_get,
    &post_api_config_post,
    &get_api_cards_get,
    &get_api_cards_post,
    &get_api_cards_delete,
    &get_api_alarms_get,
    &get_api_alarms_delete,
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

static esp_err_t handler_get_favicon(httpd_req_t *r) {
    // 1. 设置正确的响应类型（图标文件）
    httpd_resp_set_type(r, "image/x-icon");

    // 2. 打开文件获取信息（大小）
    FILE *f = fopen("/spiffs/favicon.ico", "rb");
    if (!f) {
        ESP_LOGE(TAG, "无法打开favicon.ico");
        httpd_resp_set_status(r, "404 Not Found");
        httpd_resp_sendstr(r, "favicon not found");
        return ESP_FAIL;
    }

    // 3. 获取文件大小
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);  // 回到文件开头

    if (file_size <= 0 || file_size > 1024 * 1024) {  // 限制最大1MB，防止内存溢出
        fclose(f);
        ESP_LOGE(TAG, "无效的favicon大小: %ld", file_size);
        httpd_resp_set_status(r, "500 Internal Error");
        httpd_resp_sendstr(r, "invalid favicon");
        return ESP_FAIL;
    }

    // 4. 动态分配缓冲区（或使用栈缓冲区，根据实际大小调整）
    uint8_t *buf = malloc(file_size);
    if (!buf) {
        fclose(f);
        ESP_LOGE(TAG, "内存分配失败");
        httpd_resp_set_status(r, "500 Internal Error");
        httpd_resp_sendstr(r, "memory error");
        return ESP_ERR_NO_MEM;
    }

    // 5. 读取文件内容
    size_t bytes_read = fread(buf, 1, file_size, f);
    fclose(f);

    if (bytes_read != file_size) {
        free(buf);
        ESP_LOGE(TAG, "文件读取不完整: %zu/%ld", bytes_read, file_size);
        httpd_resp_set_status(r, "500 Internal Error");
        httpd_resp_sendstr(r, "read error");
        return ESP_FAIL;
    }

    // 6. 发送完整的二进制数据（使用实际文件大小）
    esp_err_t response = httpd_resp_send(r, (const char*)buf, file_size);
    free(buf);  // 释放缓冲区

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

/**
  * @brief  http_handler_ping
  * @param  r http请求句柄
  * @retval ESP_OK - 成功，其他失败
  * @note   客户端页面每次加载完毕（首次打开/刷新），都会触发一次ping的http请求，用于
  * 鉴别与服务器的连接状态
  */
static esp_err_t handler_ping(httpd_req_t *r) {
    // 设置响应状态码为 200（成功），确保 response.ok 为 true
    httpd_resp_set_status(r, "200 OK");
    // 设置响应类型（纯文本）
    httpd_resp_set_type(r, "text/plain");
    // 发送简单响应内容
    const char *resp = "pong";
    httpd_resp_send(r, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



static esp_err_t handler_get_api_status(httpd_req_t *r) {
    // 1. 创建根JSON对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        httpd_resp_send_500(r);  // 内存分配失败时返回500错误
        return ESP_FAIL;
    }

    // 2. 从全局变量读取数据并添加到JSON对象
    // 注意：volatile变量访问需确保线程安全（必要时加锁）
    cJSON_AddNumberToObject(root, "charge_status", g_running_info.charge_status);
    cJSON_AddNumberToObject(root, "power", g_running_info.power);
    cJSON_AddNumberToObject(root, "voltage", g_running_info.voltage);
    cJSON_AddNumberToObject(root, "current", g_running_info.current);
    cJSON_AddNumberToObject(root, "net_status", g_running_info.net_status);

    // 3. 将JSON对象转换为字符串
    char *json_str = cJSON_PrintUnformatted(root);  // 无格式（紧凑）输出
    if (json_str == NULL) {
        cJSON_Delete(root);
        httpd_resp_send_500(r);
        return ESP_FAIL;
    }

    // 4. 发送HTTP响应
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_send(r, json_str, HTTPD_RESP_USE_STRLEN);

    // 5. 释放cJSON分配的内存（避免内存泄漏）
    cJSON_Delete(root);
    free(json_str);  // cJSON_Print系列函数返回的字符串需手动释放

    return ESP_OK;
}


static esp_err_t handler_get_api_config(httpd_req_t *r) {
    // 1. 创建根JSON对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        httpd_resp_send_500(r);  // 内存分配失败时返回500错误
        return ESP_FAIL;
    }

    // 2. 从全局变量读取数据并添加到JSON对象
    // 注意：volatile变量访问需确保线程安全（必要时加锁）
    cJSON_AddNumberToObject(root, "ov_threshold", g_param_config.ov_threshold);
    cJSON_AddNumberToObject(root, "uv_threshold", g_param_config.uv_threshold);
    cJSON_AddNumberToObject(root, "leakagedc", g_param_config.leakagedc);
    cJSON_AddNumberToObject(root, "leakageac", g_param_config.leakageac);
    cJSON_AddNumberToObject(root, "maxcc", g_param_config.maxcc);

    // 3. 将JSON对象转换为字符串
    char *json_str = cJSON_PrintUnformatted(root);  // 无格式（紧凑）输出
    if (json_str == NULL) {
        cJSON_Delete(root);
        httpd_resp_send_500(r);
        return ESP_FAIL;
    }

    // 4. 发送HTTP响应
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_send(r, json_str, HTTPD_RESP_USE_STRLEN);

    // 5. 释放cJSON分配的内存（避免内存泄漏）
    cJSON_Delete(root);
    free(json_str);  // cJSON_Print系列函数返回的字符串需手动释放

    return ESP_OK;
}

static esp_err_t handler_post_api_config(httpd_req_t *r) {
    // 1. 读取前端发送的 JSON 数据
    char buf[1024];
    ssize_t len = httpd_req_recv(r, buf, sizeof(buf)-1);
    if (len <= 0) {
        // 读取失败，返回 400 错误（手动设置状态码）
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_set_type(r, "application/json");
        const char *error_resp = "{\"success\": false, \"msg\": \"读取请求数据失败\"}";
        httpd_resp_send(r, error_resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    buf[len] = '\0'; // 确保字符串结束
    ESP_LOGI(TAG, "收到配置数据: %s", buf);

    // 2. 解析 JSON 并保存到存储（实际应用中补充）

    // 3. 返回成功响应（200 状态码）
    const char *resp = "{\"success\": true}";
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_send(r, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 示例：删除卡片失败时的响应
static esp_err_t handler_api_cards_delete(httpd_req_t *r) {
    // 解析卡片 ID（省略）
    const char *card_id = "12345678"; // 示例 ID

    // 假设删除失败（如卡片不存在）
    bool delete_success = false; 
    if (!delete_success) {
        // 返回 400 错误（Bad Request）
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_set_type(r, "application/json");
        const char *resp = "{\"success\": false, \"msg\": \"卡片不存在\"}";
        httpd_resp_send(r, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    // 成功删除（返回 200）
    const char *resp = "{\"success\": true}";
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_send(r, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

#include <cJSON.h>
#include <string.h>

// 假设使用全局数组模拟卡片存储（实际应使用 NVS 或文件系统持久化）
typedef struct {
    char id[9];         // 8位卡号 + 结束符
    char expireDate[11]; // 日期格式：YYYY-MM-DD
} AuthCard;
static AuthCard g_card_list[100] = {0}; // 最大100张卡
static int g_card_count = 0;            // 当前卡片数量

static esp_err_t handler_api_cards_get(httpd_req_t *r) 
{
    // 1. 创建JSON数组
    cJSON *root = cJSON_CreateArray();
    if (root == NULL) {
        httpd_resp_set_status(r, "500 Internal Server Error");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"JSON创建失败\"}");
        return ESP_FAIL;
    }

    // 2. 填充卡片数据到JSON数组
    for (int i = 0; i < g_card_count; i++) {
        cJSON *card = cJSON_CreateObject();
        cJSON_AddStringToObject(card, "id", g_card_list[i].id);
        cJSON_AddStringToObject(card, "expireDate", g_card_list[i].expireDate);
        cJSON_AddItemToArray(root, card);
    }

    // 3. 转换JSON为字符串并发送
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        httpd_resp_set_status(r, "500 Internal Server Error");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"JSON序列化失败\"}");
        return ESP_FAIL;
    }

    // 设置响应头并发送
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_send(r, json_str, HTTPD_RESP_USE_STRLEN);

    // 释放资源
    cJSON_Delete(root);
    free(json_str);
    return ESP_OK;
}

static esp_err_t handler_api_cards_post(httpd_req_t *r) {
    // 1. 读取请求体（前端发送的JSON）
    char buf[128] = {0};
    ssize_t len = httpd_req_recv(r, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"读取请求数据失败\"}");
        return ESP_FAIL;
    }

    // 2. 解析JSON
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"JSON格式错误\"}");
        return ESP_FAIL;
    }

    // 3. 提取卡号和有效期
    cJSON *id = cJSON_GetObjectItem(root, "id");
    cJSON *expire = cJSON_GetObjectItem(root, "expireDate");
    if (id == NULL || expire == NULL || !cJSON_IsString(id) || !cJSON_IsString(expire)) {
        cJSON_Delete(root);
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"缺少卡号或有效期\"}");
        return ESP_FAIL;
    }

    // 验证卡号为8位数字
    if (strlen(id->valuestring) != 8) {
        cJSON_Delete(root);
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"卡号必须为8位数字\"}");
        return ESP_FAIL;
    }

    // 4. 保存卡片（检查是否已满）
    if (g_card_count >= 100) {
        cJSON_Delete(root);
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"卡片数量已达上限\"}");
        return ESP_FAIL;
    }

    // 复制数据到全局数组
    strncpy(g_card_list[g_card_count].id, id->valuestring, 8);
    strncpy(g_card_list[g_card_count].expireDate, expire->valuestring, 10);
    g_card_count++;

    // 5. 返回成功响应
    cJSON_Delete(root);
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_sendstr(r, "{\"success\": true}");
    return ESP_OK;
}

// 假设使用全局数组模拟告警存储
typedef struct {
    char time[20];       // 时间格式：YYYY-MM-DD HH:MM:SS
    char coverStatus[5]; // "open" 或 "closed"
    bool handled;        // 是否处理
} AlarmRecord;
static AlarmRecord g_alarm_list[50] = {0}; // 最大50条记录
static int g_alarm_count = 0;              // 当前告警数量

static esp_err_t handler_api_alarms_get(httpd_req_t *r) {
    // 1. 创建JSON数组
    cJSON *root = cJSON_CreateArray();
    if (root == NULL) {
        httpd_resp_set_status(r, "500 Internal Server Error");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"JSON创建失败\"}");
        return ESP_FAIL;
    }

    // 2. 填充告警数据到JSON数组
    for (int i = 0; i < g_alarm_count; i++) {
        cJSON *alarm = cJSON_CreateObject();
        cJSON_AddStringToObject(alarm, "time", g_alarm_list[i].time);
        cJSON_AddStringToObject(alarm, "coverStatus", g_alarm_list[i].coverStatus);
        cJSON_AddBoolToObject(alarm, "handled", g_alarm_list[i].handled);
        cJSON_AddItemToArray(root, alarm);
    }

    // 3. 转换JSON为字符串并发送
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        httpd_resp_set_status(r, "500 Internal Server Error");
        httpd_resp_sendstr(r, "{\"success\": false, \"msg\": \"JSON序列化失败\"}");
        return ESP_FAIL;
    }

    // 设置响应头并发送
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_send(r, json_str, HTTPD_RESP_USE_STRLEN);

    // 释放资源
    cJSON_Delete(root);
    free(json_str);
    return ESP_OK;
}

static esp_err_t handler_api_alarms_delete(httpd_req_t *r) {
    // 清空告警列表（实际应用中需同步清除持久化存储）
    memset(g_alarm_list, 0, sizeof(g_alarm_list));
    g_alarm_count = 0;

    // 返回成功响应
    httpd_resp_set_status(r, "200 OK");
    httpd_resp_set_type(r, "application/json");
    httpd_resp_sendstr(r, "{\"success\": true}");
    return ESP_OK;
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
    config.max_uri_handlers = 16;  // 最大URI处理程序数量

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