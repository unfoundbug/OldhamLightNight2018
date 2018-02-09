#ifndef __cplusplus
#define nullptr  NULL
#endif

#define HIGH 1
#define LOW 0
#define OUTPUT GPIO_MODE_OUTPUT
#define INPUT GPIO_MODE_INPUT

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define floor(a)   ((int)(a))
#define ceil(a)    ((int)((int)(a) < (a) ? (a+1) : (a)))


uint32_t IRAM_ATTR millis()
{
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void delay(uint32_t ms)
{
  if (ms > 0) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
  }
}