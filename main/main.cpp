#include "keys.h"

#include <cstring>
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "ESP32NetIF.h"
#include "MyVehicle.h"
#include "json.hpp"

using json = nlohmann::json;

extern const uint8_t index_min_html_start[] asm("_binary_index_min_html_start");
extern const uint8_t index_min_html_end[] asm("_binary_index_min_html_end");
const unsigned long index_min_html_len = strlen((const char*)index_min_html_start);

extern const uint8_t chart_min_js_start[] asm("_binary_Chart_min_js_start");
extern const uint8_t chart_min_js_end[] asm("_binary_Chart_min_js_end");
const unsigned long chart_min_js_len = strlen((const char*)chart_min_js_start);

extern const uint8_t stylesheet_min_css_start[] asm("_binary_stylesheet_min_css_start");
extern const uint8_t stylesheet_min_css_end[] asm("_binary_stylesheet_min_css_end");
const unsigned long stylesheet_min_css_len = strlen((const char*)stylesheet_min_css_start);

extern const uint8_t core_min_js_start[] asm("_binary_core_min_js_start");
extern const uint8_t core_min_js_end[] asm("_binary_core_min_js_end");
const unsigned long core_min_js_len = strlen((const char*)core_min_js_start);

static const char* TAG = "IPCar";
MyVehicle* car;

void setupWiFiAP(const char* ssid, const char* password) {
    wifi_config_t ap_config;
    memset(&ap_config, 0, sizeof(ap_config));
    memcpy(ap_config.ap.ssid, ssid, strlen(ssid));
    memcpy(ap_config.ap.password, password, strlen(password));
    ap_config.ap.channel = 0;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.ssid_hidden = 0;
    ap_config.ap.max_connection = 1;
    //ap_config.ap.beacon_interval = 100;
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
}

void setupWiFiSTA(const char* ssid, const char* password) {
    wifi_config_t sta_config;
    memset(&sta_config, 0, sizeof(sta_config));
    memcpy(sta_config.sta.ssid, ssid, strlen(ssid));
    memcpy(sta_config.sta.password, password, strlen(password));
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
}

std::string handleCmd(std::string cmd, void* param) {
    MyVehicle* vcar = (MyVehicle*)param;
    std::string res;
    auto rootIN = json::parse(cmd);
    json rootOUT;

    if (!rootIN.at("cmd").get<std::string>().compare("ping")) {
        rootOUT["Success"] = (bool)true;
    }

    else if (!rootIN.at("cmd").get<std::string>().compare("reset")) {
        esp_restart();
    }

    else if (!rootIN.at("cmd").get<std::string>().compare("run")) {
        try {
            vcar->run((direction_t)(int)rootIN["args"][0], (unsigned int)rootIN["args"][1], (float)rootIN["args"][2]);
            rootOUT["Success"] = (bool)true;
        } catch (std::exception e) {
            rootOUT["Success"] = (bool)false;
        }
    }

    else if (!rootIN.at("cmd").get<std::string>().compare("getTelemetry")) {
        rootOUT["success"] = (bool)true;
        for (int i = 0; 1 == 1; i++) {
            telemetry_data_t currentTelemetryData;
            if (xQueueReceive(vcar->telemetry_records, &currentTelemetryData, 0) != pdPASS) {
                break;
            }
            rootOUT["telemetry"][i]["uptime"] = (std::string)std::to_string(currentTelemetryData.uptime);
            rootOUT["telemetry"][i]["errorCode"] = (int)currentTelemetryData.errorCode;
            rootOUT["telemetry"][i]["throttle"] = currentTelemetryData.throttle;
            rootOUT["telemetry"][i]["direction"] = currentTelemetryData.direction;
            rootOUT["telemetry"][i]["steering"] = currentTelemetryData.steering;

            rootOUT["telemetry"][i]["imu"]["success"] = currentTelemetryData.imu.success;
            rootOUT["telemetry"][i]["imu"]["orientation_mode"] = currentTelemetryData.imu.orientation_mode;

            rootOUT["telemetry"][i]["imu"]["lia"]["x"] = currentTelemetryData.imu.lia.x;
            rootOUT["telemetry"][i]["imu"]["lia"]["y"] = currentTelemetryData.imu.lia.y;
            rootOUT["telemetry"][i]["imu"]["lia"]["z"] = currentTelemetryData.imu.lia.z;

            rootOUT["telemetry"][i]["imu"]["euler"]["x"] = currentTelemetryData.imu.euler.x;
            rootOUT["telemetry"][i]["imu"]["euler"]["y"] = currentTelemetryData.imu.euler.y;
            rootOUT["telemetry"][i]["imu"]["euler"]["z"] = currentTelemetryData.imu.euler.z;

            rootOUT["telemetry"][i]["battery"]["success"] = currentTelemetryData.battery.success;
            rootOUT["telemetry"][i]["battery"]["percentage"] = currentTelemetryData.battery.percentage;
            rootOUT["telemetry"][i]["battery"]["voltage"] = currentTelemetryData.battery.voltage;
            rootOUT["telemetry"][i]["battery"]["state"] = currentTelemetryData.battery.state;

            for (int wi = 0; wi < 4; wi++) {
                rootOUT["telemetry"][i]["wheels"][wi]["success"] = currentTelemetryData.wheels[i].success;
                rootOUT["telemetry"][i]["wheels"][wi]["speed"] = currentTelemetryData.wheels[i].speed;
                rootOUT["telemetry"][i]["wheels"][wi]["distance"] = currentTelemetryData.wheels[i].distance;
                rootOUT["telemetry"][i]["wheels"][wi]["rpm"] = currentTelemetryData.wheels[i].rpm;
                rootOUT["telemetry"][i]["wheels"][wi]["diameter"] = currentTelemetryData.wheels[i].diameter;
            }
        }
    }

    res = rootOUT.dump();
    return res;
}

void NetIFHandler(int client, char* data, unsigned int len) {
    if (len == 0) {
        try {
            car->run(Directions::Release, 0, 0);
        } catch (std::exception& e) {
        }
    }
    char contentLen[21];
    if (!strncmp(data, "GET / ", 6)) {
        itoa(index_min_html_len, contentLen, 10);

        send(client, HEADERS_OK_HTML_200, HEADERS_OK_HTML_200_LEN, 0);
        send(client, contentLen, strlen(contentLen), 0);
        send(client, "\r\n\r\n", 4, 0);
        send(client, (const char*)index_min_html_start, index_min_html_len, 0);
        return;
    } else if (!strncmp(data, "GET /Chart.min.js ", 18)) {
        itoa(chart_min_js_len, contentLen, 10);

        send(client, HEADERS_OK_JS_200, HEADERS_OK_JS_200_LEN, 0);
        send(client, contentLen, strlen(contentLen), 0);
        send(client, "\r\n\r\n", 4, 0);
        send(client, (const char*)chart_min_js_start, chart_min_js_len, 0);
        return;
    }

    else if (!strncmp(data, "GET /core.min.js ", 13)) {
        itoa(core_min_js_len, contentLen, 10);

        send(client, HEADERS_OK_JS_200, HEADERS_OK_JS_200_LEN, 0);
        send(client, contentLen, strlen(contentLen), 0);
        send(client, "\r\n\r\n", 4, 0);
        send(client, (const char*)core_min_js_start, core_min_js_len, 0);
        return;
    } else if (!strncmp(data, "GET /stylesheet.min.css ", 20)) {
        itoa(stylesheet_min_css_len, contentLen, 10);

        send(client, HEADERS_OK_CSS_200, HEADERS_OK_CSS_200_LEN, 0);
        send(client, contentLen, strlen(contentLen), 0);
        send(client, "\r\n\r\n", 4, 0);
        send(client, (const char*)stylesheet_min_css_start, stylesheet_min_css_len, 0);
        return;
    } else if (!strncmp(data, "GET /ws ", 8)) {
        ESP32NetIF::handleWS(client, data, len, &handleCmd, car);
        return;
    } else {
        send(client, PAGE_NOT_FOUND_404, PAGE_NOT_FOUND_404_LEN, 0);
        return;
    }
}

static esp_err_t systemEventHandler(void* ctx, system_event_t* event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(TAG, "AP Enabled.");
            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "New APClient Connected.");
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "APClient Disconnected.");
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "got ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            esp_wifi_connect();
            break;

        default:
            break;
    }
    return ESP_OK;
}

extern "C" void app_main(void) {
    nvs_flash_init();
    tcpip_adapter_init();

    car = new MyVehicle();
    car->run(Directions::Release, 0, 0);

    ESP32NetIF ssw(80, &NetIFHandler);
    ssw.serveForever();

    //Init WiFi
    wifi_init_config_t wifiInitializationConfig = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifiInitializationConfig);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    setupWiFiSTA(WiFi_STA_SSID, WiFi_STA_PWD);

    esp_event_loop_init(systemEventHandler, car);

    esp_wifi_start();
    esp_wifi_connect();
}