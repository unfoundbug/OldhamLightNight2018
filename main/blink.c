/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "driver/rmt.h"
#include "nvs.h"
#include "nvs_flash.h"


#include <math.h>
#include "esp32_digital_led_lib.h"

#include "Generics.h"
#include "WiFiConfig.h"
#include "TCPServer.h"
#include "ByteMath.h"
static const char *TAG = "UFB.WifiControl";
#define BLINK_GPIO 2

strand_t STRANDS[] = { // Avoid using any of the strapping pins on the ESP32
  {.rmtChannel = 1, .gpioNum = 13, .ledType = LED_WS2812BW, .brightLimit = 255, .numPixels =  93,
   .pixels = nullptr, ._stateVars = nullptr},
};

int STRANDCNT = sizeof(STRANDS)/sizeof(STRANDS[0]);
pixelColor_t colCur[64];
pixelColor_t colTarget[64];

struct STCPServer sServer;


void gpioSetup(int gpioNum, int gpioMode, int gpioVal) {
    gpio_num_t gpioNumNative = (gpio_num_t)(gpioNum);
    gpio_mode_t gpioModeNative = (gpio_mode_t)(gpioMode);
    gpio_pad_select_gpio(gpioNumNative);
    gpio_set_direction(gpioNumNative, gpioModeNative);
    gpio_set_level(gpioNumNative, gpioVal);
};

void rainbow(strand_t * pStrand, unsigned long delay_ms, unsigned long timeout_ms)
{
  const uint8_t color_div = 4;
  const uint8_t anim_step = 1;
  const uint8_t anim_max = pStrand->brightLimit - anim_step;
  pixelColor_t color1 = pixelFromRGB(anim_max, 0, 0);
  pixelColor_t color2 = pixelFromRGB(anim_max, 0, 0);
  uint8_t stepVal1 = 0;
  uint8_t stepVal2 = 0;
  bool runForever = (timeout_ms == 0 ? true : false);
  unsigned long start_ms = millis();
  while (runForever || (millis() - start_ms < timeout_ms)) {
    color1 = color2;
    stepVal1 = stepVal2;
    for (uint16_t i = 0; i < pStrand->numPixels; i++) {
      pStrand->pixels[i] = pixelFromRGB(color1.r/color_div, color1.g/color_div, color1.b/color_div);
      if (i == 1) {
        color2 = color1;
        stepVal2 = stepVal1;
      }
      switch (stepVal1) {
        case 0:
        color1.g += anim_step;
        if (color1.g >= anim_max)
          stepVal1++;
        break;
        case 1:
        color1.r -= anim_step;
        if (color1.r == 0)
          stepVal1++;
        break;
        case 2:
        color1.b += anim_step;
        if (color1.b >= anim_max)
          stepVal1++;
        break;
        case 3:
        color1.g -= anim_step;
        if (color1.g == 0)
          stepVal1++;
        break;
        case 4:
        color1.r += anim_step;
        if (color1.r >= anim_max)
          stepVal1++;
        break;
        case 5:
        color1.b -= anim_step;
        if (color1.b == 0)
          stepVal1 = 0;
        break;
      }
    }
    digitalLeds_updatePixels(pStrand);
    delay(delay_ms);
  }
  digitalLeds_resetPixels(pStrand);
}
const int kiLayer1 = 32;
int iLayer1Bass;
const int kiLayer2 = 32+24;
int iLayer2Bass;
const int kiLayer3 = 32+24+16;
int iLayer3Bass;
const int kiLayer4 = 32+24+16+12;
int iLayer4Bass;
uint32_t pLevel = 0;

bool bFadeBass = false;
int iGetMax(int i, int j)
{
 return	i > j ? i : j;
}
void looptask(void *pvParameter)
{
	char cBuffer[2048];
	unsigned int iReadData;
	while(true)
	{
		if(TCPS_AcceptSocket(&sServer))
		{
			memset(cBuffer, 0, 2048);
			ESP_LOGE(TAG, "Has socket");
			do
			{				
				iReadData = TCPS_RecieveData(&sServer, (void*) cBuffer, 2048);
				if(iReadData > 0)
				{
					int j = 1;
					for (uint16_t i = 0; i < STRANDS[0].numPixels; i++) {
						colTarget[i] = pixelFromRGB(cBuffer[j], cBuffer[j+1], cBuffer[j+2]);
						if(bFadeBass)
						{
							bFadeBass = false;
							
							iLayer1Bass = iGetMax(iLayer2Bass, HQAverage((uint8_t)iLayer1Bass, (uint8_t)0));
							iLayer2Bass = iGetMax(iLayer3Bass, HQAverage((uint8_t)iLayer2Bass, (uint8_t)0));
							iLayer3Bass = iGetMax(iLayer4Bass, HQAverage((uint8_t)iLayer3Bass, (uint8_t)0));
							iLayer4Bass = colTarget[kiLayer4].w;					
							
						}
						if(i < kiLayer1)
								colTarget[i].w = iLayer1Bass;
							else if(i < kiLayer2)
								colTarget[i].w = iLayer2Bass;
							else if (i < kiLayer3)
								colTarget[i].w = iLayer3Bass;
							else if(i < kiLayer4)
								colTarget[i].w = iLayer4Bass;
						if(i > STRANDS[0].numPixels - 10)
							colTarget[i].w = cBuffer[0];
						j+=3;
					}
					vTaskDelay(1);
				}
			} while(iReadData > 0);
		}
		ESP_LOGI(TAG, "Task done");
		vTaskDelay(1);
	}

}

void looptask2(void* pvParam)
{
	while(true)
	{
		if(true)//rmt_wait_tx_done(STRANDS[0].rmtChannel-1, 1) == ESP_OK)
		{
			for (uint16_t i = 0; i < STRANDS[0].numPixels; i++) {
				STRANDS[0].pixels[i] = colTarget[i];
			}
			digitalLeds_updatePixels(&STRANDS[0]);
			bFadeBass = true;
		}
		else
		{
			ESP_LOGI(TAG, "Holding draw");
		
		}
		
		vTaskDelay(1);
	}
	
}


void app_main()
{
 gpioSetup(18, OUTPUT, LOW);
 gpioSetup(13, OUTPUT, LOW);
     esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
			
	ESP_LOGI(TAG, "Initialising NVS");
	nvs_flash_init();
			
	ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
	
	
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
        }
        return;
    }
	
	if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
		ets_printf("Init FAILURE: halting\n");
		while (true) {};
	}	
			
	if(!LoadWifiSettings())
		CreateDefaultSettings();
	ApplyWifiSettings();
	
	
	TCPS_StartServer(8080, &sServer);
	
	xTaskCreatePinnedToCore(&looptask, "looptask", 4096, NULL, 5, NULL, 0);
	xTaskCreatePinnedToCore(&looptask2, "looptask2", 4096, NULL, 5, NULL, 1);
	

}
