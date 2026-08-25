#include "lora_tx_task.h"

#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora_link.h"
#include <inttypes.h>
#include <string.h>

#define TAG "lora_tx"

#define LORA_TEST_PACKET_LEN 12

void lora_tx_task_run(void *argument)
{
	uint32_t sequence = 0;
	(void)argument;
	while (true) {
		uint8_t packet[LORA_TEST_PACKET_LEN] = {0};
		uint32_t uptime = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000U);
		memcpy(packet, &sequence, sizeof(sequence));
		memcpy(&packet[4], &uptime, sizeof(uptime));
		if (lora_link_send(packet, sizeof(packet)) == ESP_OK) {
			ESP_LOGI(TAG, "TRANSMIT seq=%" PRIu32 " uptime=%" PRIu32 "s", sequence, uptime);
			ESP_ERROR_CHECK(display_show_tx(sequence));
		} else {
			ESP_LOGE(TAG, "TRANSMIT failed seq=%" PRIu32, sequence);
		}
		sequence++;
		vTaskDelay(pdMS_TO_TICKS(g_lora_tx_interval_ms));
	}
}
