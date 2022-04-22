#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "Arduino.h"
#include "SIM7600.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "secrets.h"

#define MQTT_CONNECT
#define DEVICE_TAG "GPS Tracker Prototype"
#define SIM7600_TAG "SIM7600"
#define SSL_TAG "SSL"
#define MQTT_TAG "MQTT"

SIM7600 sim7600(Serial2);
GPS gps(Serial2);
MQTT mqtt(Serial2);
SSL ssl(Serial2);

TaskHandle_t Task_fetchGPS_pubMQTT;

TaskHandle_t Task_LED_Control;

TaskHandle_t Task_Battery_Monitor;

TaskHandle_t Task_Serial;

TaskHandle_t Task_Clock;

SemaphoreHandle_t Semaphore_LED_blink_count = xSemaphoreCreateBinary();

gpio_num_t LED = GPIO_NUM_27;
gpio_num_t BATTERY_MONITOR_EN = GPIO_NUM_13;

const double slope = 5.70;
const double BATT_min = 6.00;

const unsigned int AWS_update_interval_ms = 5000;

bool active_hours = true;
const unsigned int aws_port = 8883;
uint8_t LED_blink_count = 1;

/**
 * @brief Function to calculate the battery voltage using the voltage divider with MOSFET. 12-bit resolution.
 * 
 * 
 * @return double - Voltage of battery.
 */
double battery_voltage()
{
	gpio_set_level(BATTERY_MONITOR_EN, 1);
	vTaskDelay(50 / portTICK_PERIOD_MS);

	esp_adc_cal_characteristics_t adc_chars;

	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, 1100, &adc_chars);

	uint32_t voltage;
	esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adc_chars, &voltage);

	gpio_set_level(BATTERY_MONITOR_EN, 0);
	return (slope * (voltage / 1000.0) ) + 0.075;
}

/**
 * @brief Function to end the MQTT session.
 * 
 */
void endMQTT()
{
	#ifdef MQTT_CONNECT
	mqtt.disconnect() ? ESP_LOGI(MQTT_TAG, "MQTT broker disconnected") : ESP_LOGE(MQTT_TAG, "MQTT broker disconnection failed.");
	mqtt.releaseClient() ? ESP_LOGI(MQTT_TAG, "MQTT client released") : ESP_LOGE(MQTT_TAG, "MQTT client not released");
	mqtt.end() ? ESP_LOGI(MQTT_TAG, "MQTT session ended successfully") : ESP_LOGE(MQTT_TAG, "MQTT session did not end");
	#endif
}

/**
 * @brief Update the active_hours boolean based on the current time and active time.
 * 
 * @todo Implementation
 * 
 * @param parameter 
 */
void update_active_hours(void * parameter)
{
	while(true)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/**
 * @brief Task to monitor the battery voltage and switch ON/OFF the SIM module.
 * 
 * @param parameter 
 */
void battery_monitor(void * parameter)
{
	gpio_reset_pin(BATTERY_MONITOR_EN);
	gpio_set_direction(BATTERY_MONITOR_EN, GPIO_MODE_OUTPUT);

	adc_set_clk_div(1);
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_6);

	while(true)
	{
		if ( battery_voltage() < BATT_min + 0.3 || !active_hours)
		{
			vTaskSuspendAll();
			endMQTT();
			sim7600.shutdown();
			vTaskDelay(200 / portTICK_PERIOD_MS);
			sim7600.powerOFF();
			xSemaphoreTake(Semaphore_LED_blink_count, portMAX_DELAY);
			LED_blink_count = 3;
			xSemaphoreGive(Semaphore_LED_blink_count);

			while(true)
			{
				if ( !active_hours )
					vTaskResume(Task_LED_Control);
				vTaskDelay(2000 / portTICK_PERIOD_MS);
			}
		}
		else
		{
			sim7600.powerON();
		}
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

/**
 * @brief Task to blink the LED based on the count.
 * 
 * @param parameter 
 */
void blink_LED(void *parameter)
{	
	gpio_reset_pin(LED);
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);

	uint8_t blink_count;

	while (true)
	{
		gpio_set_level(LED, 0);
		vTaskSuspend(NULL);
		
		xSemaphoreTake(Semaphore_LED_blink_count, portMAX_DELAY);
		blink_count = LED_blink_count;
		xSemaphoreGive(Semaphore_LED_blink_count);

		for (uint8_t i = 0; i < blink_count; i++)
		{
			gpio_set_level(LED, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(LED, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	}
}

/**
 * @brief Function to configure the SSL context and connect to the AWS MQTT broker.
 * 
 */
void configureSSL_MQTT()
{
	#ifdef MQTT_CONNECT
	ssl.checkCertificates(cacert, clientcert, clientkey) ? ESP_LOGI(SSL_TAG, "Certificates present") : ESP_LOGE(SSL_TAG, "Certificates not found");
	ssl.configureSSL(cacert, clientcert, clientkey) ? ESP_LOGI(SSL_TAG, "SSL configured successfully") : ESP_LOGE(SSL_TAG, "SSL configuration failed");

	mqtt.begin() ? ESP_LOGI(MQTT_TAG, "MQTT session started successfully") : ESP_LOGE(MQTT_TAG, "MQTT session did not start");
	mqtt.acquireClient() ? ESP_LOGI(MQTT_TAG, "MQTT client acquired successfully") : ESP_LOGE(MQTT_TAG, "MQTT client was not acquired");
	mqtt.setSSLContext() ? ESP_LOGI(MQTT_TAG, "MQTT SSL context set successfully") : ESP_LOGE(MQTT_TAG, "MQTT SSL context was not set");
	mqtt.disconnect();
	bool success = mqtt.connect(aws_server, aws_port) ? ESP_LOGI(MQTT_TAG, "MQTT broker connected successfully") : ESP_LOGE(MQTT_TAG, "Could not connect to MQTT broker");

	xSemaphoreTake(Semaphore_LED_blink_count, portMAX_DELAY);
	LED_blink_count = success ? 1 : 3;
	const unsigned int delay_interval = success ? 300 : 1000;
	xSemaphoreGive(Semaphore_LED_blink_count);

	vTaskResume(Task_LED_Control);
	vTaskDelay(delay_interval / portTICK_PERIOD_MS);
	#endif
}

/**
 * @brief Task to Fetch GPS data and Publish MQTT message
 * 
 * @param parameter 
 */
void fetchGPS_pubMQTT(void *parameter)
{
	Serial.println("Latitude\tLongitude\tAltitude\tSpeed\t\tCourse\t\tEpoch Time");

	while (true)
	{
		if ( gps.getData() )
		{
			Serial.printf("Latitude:\t%lf\nLongitude:\t%lf\n", gps.data.latitude, gps.data.longitude);
			Serial.printf("Altitude:\t%lf\nSpeed:\t\t%lf\n", gps.data.altitude, gps.data.speed);
			Serial.printf("Course:\t\t%lf\nEpoch Time:\t%ld\n", gps.data.course, gps.data.timestamp);

			uint8_t mac[6];
			esp_read_mac(mac, ESP_MAC_WIFI_STA);

			char publishTopic[20];

			sprintf(publishTopic, "sim7600/pub");

			char payload[150];
			snprintf(payload, sizeof(payload), "{\"latitude\":%.8lf,\"longitude\":%.8lf,\"speed\":%.2lf,\"course\":%.2lf,\"timestamp\":%li,\"battery\":%.2lf}",
					 gps.data.latitude, gps.data.longitude, gps.data.speed, gps.data.course, gps.data.timestamp, battery_voltage());

			Serial.printf("%u-%s\n", strlen(publishTopic), publishTopic);
			Serial.printf("%u-%s\n", strlen(payload), payload);
			#ifdef MQTT_CONNECT
			mqtt.setPublishTopicPayload(publishTopic, payload);
			bool success = mqtt.publish();

			xSemaphoreTake(Semaphore_LED_blink_count, portMAX_DELAY);
			LED_blink_count = success ? 1 : 3;
			const unsigned int delay_interval = success ? 300 : 1000;
			xSemaphoreGive(Semaphore_LED_blink_count);

			vTaskResume(Task_LED_Control);
			vTaskDelay(delay_interval / portTICK_PERIOD_MS);
			#endif
		}
		else
		{
			xSemaphoreTake(Semaphore_LED_blink_count, portMAX_DELAY);
			LED_blink_count = 2;
			const unsigned int delay_interval = 650;
			xSemaphoreGive(Semaphore_LED_blink_count);

			vTaskResume(Task_LED_Control);
			vTaskDelay(delay_interval / portTICK_PERIOD_MS);
			Serial.println("Invalid Data or Module is not Switched ON or MQTT disabled\n");
		}

		vTaskDelay(AWS_update_interval_ms / portTICK_PERIOD_MS);
	}
}

/**
 * @brief Task to monitor the output of SIM7600.
 * 
 * @todo Implementation
 * 
 * @param parameter 
 */
void serial_monitor(void * parameter)
{
	char buffer[100];
	bool message = false;
	while(true)
	{
		while( !sim7600.waitingForResponse && Serial2.available() )
		{
			memset(buffer, 0, sizeof(buffer));
			Serial2.read(buffer, 100);
			message = true;
		}

		
		if ( message )
		{
			if ( strstr(buffer, "+CMQTTCONNLOST") || strstr(buffer, "+CMQTTNONET") )
			{
				vTaskDelete(Task_fetchGPS_pubMQTT);

				configureSSL_MQTT();

				xTaskCreatePinnedToCore(fetchGPS_pubMQTT, "Fetch GPS and Publish MQTT", 4096, NULL, 1, &Task_fetchGPS_pubMQTT, 1);
			}
			message = false;
		}



	}
}

#endif