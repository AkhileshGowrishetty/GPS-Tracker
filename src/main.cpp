#include <Arduino.h>
#include "SIM7600.h"
#include "secrets.h"
#include "functions.h"


void setup()
{
	Serial.begin(115200);
	Serial2.begin(115200);

	xSemaphoreGive(Semaphore_LED_blink_count);

	xTaskCreatePinnedToCore(blink_LED, "LED Blink", 2048, NULL, 1, &Task_LED_Control, 1);
	xTaskCreatePinnedToCore(battery_monitor, "Battery Monitoring Function", 2048, NULL, 1, &Task_Battery_Monitor, 1);
	
	
	
	delay(10000);
	sim7600.waitForResponse("PB DONE", 25);

	sim7600.echoOFF() ? ESP_LOGI(SIM7600_TAG, "Echo switched OFF") : ESP_LOGE(SIM7600_TAG, "Echo did not switch OFF");

	configureSSL_MQTT();

	gps.begin();

	xTaskCreatePinnedToCore(update_active_hours, "Real Time Management", 2048, NULL, 1, &Task_Clock, 1);
	xTaskCreatePinnedToCore(fetchGPS_pubMQTT, "Fetch GPS and Publish MQTT", 4096, NULL, 1, &Task_fetchGPS_pubMQTT, 1);
	//xTaskCreatePinnedToCore(serial_monitor, "Monitor Output from SIM7600", 4096, NULL, 1, &Task_Serial, 1);
}

void loop()
{

}
