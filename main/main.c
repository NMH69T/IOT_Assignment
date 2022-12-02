#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "ultrasonic.h"
#include "dht.h"

#define MAX_DISTANCE_CM 500 // 5m max

#define SENSOR_TYPE DHT_TYPE_DHT11
#define CONFIG_EXAMPLE_DATA_GPIO 17

#define TRIGGER_GPIO_1 19
#define ECHO_GPIO_1 18

void ultrasonic_test_1(void *pvParameters)
{
   ultrasonic_sensor_t sensor_1 = {
      .trigger_pin = TRIGGER_GPIO_1,
      .echo_pin = ECHO_GPIO_1
   };

   ultrasonic_init(&sensor_1);
   int currentState1 = 0;
   int previousState1 = 0;

   while (true)
   {
      float distance_1;
      esp_err_t res = ultrasonic_measure(&sensor_1, MAX_DISTANCE_CM, &distance_1);
      if (res == ESP_OK)
      {
            distance_1 = distance_1 * 100;
            printf("Distance_1: %0.02f cm\n", distance_1);
      }
      vTaskDelay(pdMS_TO_TICKS(500));
   }
}

void dht_test(void *pvParameters)
{
   float temperature, humidity;

   #ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
   gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
   #endif

   while (1)
   {
      if (dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &humidity, &temperature) == ESP_OK)
         printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
      vTaskDelay(pdMS_TO_TICKS(2000));
   }
}

void soil_test(void *pvParameters)
{
   uint32_t voltage;
   adc1_config_width(ADC_WIDTH_BIT_12);
   adc1_config_channel_atten(ADC1_CHANNEL_6,ADC_ATTEN_DB_11);

   while (1)
   {
      voltage = adc1_get_raw(ADC1_CHANNEL_6);   // GPIO_34
      printf("%d\n",voltage);
      vTaskDelay(pdMS_TO_TICKS(100));
   }
}

void light_test(void *pvParameters)
{
   uint32_t voltage;
   adc1_config_width(ADC_WIDTH_BIT_12);
   adc1_config_channel_atten(ADC1_CHANNEL_7,ADC_ATTEN_DB_11);

   while (1)
   {
      voltage = adc1_get_raw(ADC1_CHANNEL_7);   // GPIO_35      
      printf("%d\n",voltage);
      vTaskDelay(pdMS_TO_TICKS(100));
   }
}

void app_main()
{
   xTaskCreatePinnedToCore(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);
   xTaskCreatePinnedToCore(soil_test, "soil_test", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);
   xTaskCreatePinnedToCore(light_test, "light_test", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);
   xTaskCreatePinnedToCore(ultrasonic_test_1, "ultrasonic_test_1", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);   
}

