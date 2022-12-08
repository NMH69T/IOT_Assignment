#include <stdio.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "value.h"
#include "json.h"
#include "ultrasonic.h"
#include "dht.h"

#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"
#include "firebase_config.h"
#include "wifi_utils.h"

using namespace ESPFirebase;
using namespace std;
using namespace Json;

#define MAX_DISTANCE_CM 500 // 5m max

#define SENSOR_TYPE DHT_TYPE_DHT11
#define CONFIG_EXAMPLE_DATA_GPIO GPIO_NUM_17

#define TRIGGER_GPIO_1 GPIO_NUM_19
#define ECHO_GPIO_1 GPIO_NUM_18

#define MOTOR_GPIO GPIO_NUM_22
#define LED_GPIO GPIO_NUM_23

float distance_1;
float temperature, humidity;
uint32_t soil;
uint32_t light;

bool state_led = false;
bool state_motor = false;

void ultrasonic_test_1(void *pvParameters)
{
   ultrasonic_sensor_t sensor_1 = {
      .trigger_pin = TRIGGER_GPIO_1,
      .echo_pin = ECHO_GPIO_1
   };

   ultrasonic_init(&sensor_1);

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
   adc1_config_width(ADC_WIDTH_BIT_12);
   gpio_pad_select_gpio(MOTOR_GPIO);
   gpio_set_direction(MOTOR_GPIO, GPIO_MODE_OUTPUT);
   gpio_set_level(MOTOR_GPIO, 0);

   while (1)
   {
      adc1_config_channel_atten(ADC1_CHANNEL_6,ADC_ATTEN_DB_11);
      soil = adc1_get_raw(ADC1_CHANNEL_6);   // GPIO_34
      printf("=======| Soil: %d\n",soil);

      if (soil > 4000 && distance_1 > 5)
      {
         gpio_set_level(MOTOR_GPIO, 1);
      }
      else gpio_set_level(MOTOR_GPIO, 0);

      vTaskDelay(pdMS_TO_TICKS(1000));
   }
}

void light_test(void *pvParameters)
{
   adc1_config_width(ADC_WIDTH_BIT_12);
   gpio_pad_select_gpio(LED_GPIO);
   gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
   gpio_set_level(LED_GPIO, 0);

   while (1)
   {
      adc1_config_channel_atten(ADC1_CHANNEL_7,ADC_ATTEN_DB_11);
      light = adc1_get_raw(ADC1_CHANNEL_7);   // GPIO_35      
      printf("Light: %d\n",light);
      vTaskDelay(pdMS_TO_TICKS(1000));

      if (light < 10)
      {
         gpio_set_level(LED_GPIO, 1);
      }
      else gpio_set_level(LED_GPIO, 0);
   }
}

void firebase(void *pvParameters)
{
wifiInit(SSID, PASSWORD);  // blocking until it connects

   // Config and Authentication
   user_account_t account = {USER_EMAIL, USER_PASSWORD};

   FirebaseApp app = FirebaseApp(API_KEY);

   app.loginUserAccount(account);

   RTDB db = RTDB(&app, DATABASE_URL);

   

   // R"()" allow us to write string as they are without escaping the characters with backslash

   // We can put a json str directly at /person1

   std::string json_str = R"({"temp": 20, "humi": 8.56,"ultra": 10 ,"light": 1024, "soil": 1024, "state_motor": true, "state_led"= true})";
   Json::Value data;
   Json::Reader reader; 
   reader.parse(json_str, data);
   while (1)
   {
      
      temperature=temperature+0.1;
      humidity=humidity+0.1;
      soil=soil+0.1;
      light=light+0.1;
      distance_1=distance_1+0.1;

      data["temperature"] = temperature;  //
      data["humidity"] = humidity;    //
      data["soil"] = soil;    //
      data["light"] = light;    //
      data["distance_1"] = distance_1;    //
      data["state_motor"] = state_motor;    //
      data["state_led"] = state_led;    //
      
      

      db.putData("/IOT", data);   

      vTaskDelay(10000/portTICK_PERIOD_MS);

      Value root = db.getData("/IOT"); // retrieve person3 from database, set it to "" to get entire database
      FastWriter writer;
      string data_string = writer.write(root);  // convert it to json string
      cout <<data_string<< std::endl;   
   }
}

extern "C" void app_main(void)
{
   xTaskCreatePinnedToCore(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);
   xTaskCreatePinnedToCore(soil_test, "soil_test", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);
   xTaskCreatePinnedToCore(light_test, "light_test", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);
   xTaskCreatePinnedToCore(ultrasonic_test_1, "ultrasonic_test_1", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL,0);   
   xTaskCreatePinnedToCore(firebase, "firebase", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL,0);   
}

