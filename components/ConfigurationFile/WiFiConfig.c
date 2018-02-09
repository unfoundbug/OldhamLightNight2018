#include "WiFiConfig.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event_loop.h"
static const char *TAG = "UFB.WifiConfig";

#define g_kcWifiVarSize 33
struct SWifiSettings
{
	char bMode;
	char rngSTASSID[g_kcWifiVarSize], rngSTAPass[g_kcWifiVarSize], rngAPSSID[g_kcWifiVarSize], rngAPPass[g_kcWifiVarSize];
	char cSum;
};

struct SWifiSettings g_sWifiSettings;

char GetChecksum(struct SWifiSettings * sSettings)
{
	char cCheck = 0;
	cCheck += sSettings->bMode;
	for(int i = 0; i < g_kcWifiVarSize; ++i)
	{
		cCheck += sSettings->rngSTASSID[i];
		cCheck += sSettings->rngSTAPass[i];
		cCheck += sSettings->rngAPSSID[i];
		cCheck += sSettings->rngAPPass[i];
	}
	return cCheck;
};
void SetChecksum(struct SWifiSettings * sSettings)
{
	sSettings->cSum = GetChecksum(sSettings);
};
char CheckChecksum(struct SWifiSettings * sSettings)
{
	return GetChecksum(sSettings) != sSettings->cSum;
};


esp_err_t wifiEventHandler(void *ctx, system_event_t *event)
{
	esp_err_t eRes = ESP_OK;
	switch(event->event_id)
	{
		case SYSTEM_EVENT_WIFI_READY:          /**< ESP32 WiFi ready */
		case SYSTEM_EVENT_SCAN_DONE:                /**< ESP32 finish scanning AP */
		case SYSTEM_EVENT_STA_START:                /**< ESP32 station start */
		case SYSTEM_EVENT_STA_STOP:                 /**< ESP32 station stop */
		case SYSTEM_EVENT_STA_CONNECTED:            /**< ESP32 station connected to AP */
		case SYSTEM_EVENT_STA_DISCONNECTED:         /**< ESP32 station disconnected from AP */
		case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:      /**< the auth mode of AP connected by ESP32 station changed */
		case SYSTEM_EVENT_STA_GOT_IP:               /**< ESP32 station got IP from connected AP */
		case SYSTEM_EVENT_STA_LOST_IP:              /**< ESP32 station lost IP and the IP is reset to 0 */
		case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:       /**< ESP32 station wps succeeds in enrollee mode */
		case SYSTEM_EVENT_STA_WPS_ER_FAILED:        /**< ESP32 station wps fails in enrollee mode */
		case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:       /**< ESP32 station wps timeout in enrollee mode */
		case SYSTEM_EVENT_STA_WPS_ER_PIN:           /**< ESP32 station wps pin code in enrollee mode */
		case SYSTEM_EVENT_AP_START:                 /**< ESP32 soft-AP start */
		case SYSTEM_EVENT_AP_STOP:                  /**< ESP32 soft-AP stop */
		case SYSTEM_EVENT_AP_STACONNECTED:          /**< a station connected to ESP32 soft-AP */
		case SYSTEM_EVENT_AP_STADISCONNECTED:       /**< a station disconnected from ESP32 soft-AP */
		case SYSTEM_EVENT_AP_PROBEREQRECVED:        /**< Receive probe request packet in soft-AP interface */
		case SYSTEM_EVENT_GOT_IP6:                  /**< ESP32 station or ap or ethernet interface v6IP addr is preferred */
		case SYSTEM_EVENT_ETH_START:                /**< ESP32 ethernet start */
		case SYSTEM_EVENT_ETH_STOP:                 /**< ESP32 ethernet stop */
		case SYSTEM_EVENT_ETH_CONNECTED:            /**< ESP32 ethernet phy link up */
		case SYSTEM_EVENT_ETH_DISCONNECTED:         /**< ESP32 ethernet phy link down */
		case SYSTEM_EVENT_ETH_GOT_IP:               /**< ESP32 ethernet got IP from connected AP */
		case SYSTEM_EVENT_MAX:
		{}break;
	}
	return eRes;
}


char LoadWifiSettings()
{
	char cRet = 0;
	esp_err_t ret;
	size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information");
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
		//SPIFFS is open, load settings file
		ESP_LOGI(TAG, "Opening file");
		FILE* f = fopen("/spiffs/WifiSettings.bin", "r");
		if (f == NULL) {
			ESP_LOGE(TAG, "Failed to open file for writing");
		}else{
			struct SWifiSettings sFile;
			//File opened
			fgets((char*)&sFile, sizeof(struct SWifiSettings), f);
			fclose(f);
			if(CheckChecksum(&sFile))
			{
				//File OK
				cRet = 1;
				memcpy(&g_sWifiSettings, &sFile, sizeof(g_sWifiSettings));
			}
		}
		
    }
	return cRet;

}
char SaveWifiSettings()
{
	char cRet = 0;
	esp_err_t ret;
	size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information");
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
		//SPIFFS is open, load settings file
		ESP_LOGI(TAG, "Opening file");
		FILE* f = fopen("/spiffs/WifiSettings.bin", "wb");
		if (f == NULL) {
			ESP_LOGE(TAG, "Failed to open file for writing");
		}else{
			//File opened
			SetChecksum(&g_sWifiSettings);
			fwrite((char*) &g_sWifiSettings, sizeof(g_sWifiSettings), 1, f);
			fclose(f);
			cRet = 1;
		}

    }
	return cRet;

}

void CreateDefaultSettings()
{
	memset(&g_sWifiSettings, 0, sizeof(struct SWifiSettings));
	g_sWifiSettings.bMode = WIFI_MODE_AP;
	uint8_t chipID[6];
	esp_efuse_read_mac(chipID);
	sprintf(g_sWifiSettings.rngAPSSID, "ESP32_%02x%02x%02x%02x%02x%02x",
		chipID[0], chipID[1], chipID[2], chipID[3], chipID[4], chipID[5]);
	g_sWifiSettings.rngAPPass[0] = 0;
	ESP_LOGI(TAG, "Creating default: %s", g_sWifiSettings.rngAPSSID);
}

void ApplyAPMode()
{
	wifi_config_t ap_config = {
        .ap = {
        .channel = 0,
        .authmode = WIFI_AUTH_OPEN,
        .ssid_hidden = 0,
        .max_connection = 4,
        .beacon_interval = 100
        }
	};
	if(g_sWifiSettings.rngAPPass[0])
	{
		ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
		strcpy((char*)ap_config.ap.password, g_sWifiSettings.rngAPPass);
	}
	else
	{
		ap_config.ap.password[0] = 0;
	}
	strcpy((char*)ap_config.ap.ssid, g_sWifiSettings.rngAPSSID);
	esp_wifi_set_config(WIFI_IF_AP, &ap_config);
};

void ApplySTAMode()
{
	wifi_sta_config_t sta_config = {0};
	if(g_sWifiSettings.rngSTAPass[0])
	{
		strcpy((char*)sta_config.password, g_sWifiSettings.rngSTAPass);
	}
	else
		sta_config.password[0] = 0;
	strcpy((char*)sta_config.ssid, g_sWifiSettings.rngSTASSID);
	sta_config.bssid_set = false;
	esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)&sta_config);
};
void ApplyWifiSettings()
{
 //esp_log_level_set("wifi", ESP_LOG_NONE); // disable wifi driver logging
	wifi_init_config_t wifiInitializationConfig = WIFI_INIT_CONFIG_DEFAULT();
 
	tcpip_adapter_init();

	esp_wifi_init(&wifiInitializationConfig);

	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	
	wifi_mode_t wfMode = (wifi_mode_t)g_sWifiSettings.bMode;
	
	esp_wifi_set_mode(wfMode);
	switch(wfMode)
	{
		case WIFI_MODE_APSTA:
		{
			ApplyAPMode();
			ApplySTAMode();
		}break;
		case WIFI_MODE_AP:
		{
			ApplyAPMode();
		}break;
		case WIFI_MODE_STA:
		{
			ApplySTAMode();
		}break;	
		case WIFI_MODE_NULL:
		case WIFI_MODE_MAX:
		{
		}break;
	}
	
	esp_event_loop_init(wifiEventHandler, 0);
	
	esp_wifi_start();
}

void WaitWifiSettings()
{
}
