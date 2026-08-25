#include "lora_rx_task.h"

#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lora_link.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#define TAG "lora_rx"

#define LORA_TEST_PACKET_LEN 12

void lora_rx_task_run(void *argument)
{
	uint32_t expected = 0;
	bool received_any = false;
	(void)argument;
	ESP_LOGI(TAG, "RX listening for packets");
	while (true) {
		lora_frame_t frame;
		if (lora_link_receive(&frame, portMAX_DELAY) != ESP_OK) {
			ESP_LOGW(TAG, "RECEIVE failed");
			continue;
		}
		if (frame.length != LORA_TEST_PACKET_LEN) {
			ESP_LOGW(TAG, "RECEIVE invalid length=%u", frame.length);
			continue;
		}
		uint32_t sequence;
		uint32_t uptime;
		memcpy(&sequence, frame.data, sizeof(sequence));
		memcpy(&uptime, &frame.data[4], sizeof(uptime));
		if (received_any && sequence > expected) {
			ESP_LOGW(TAG, "PACKET LOSS missing=%" PRIu32, sequence - expected);
		}
		expected = sequence + 1;
		received_any = true;
		ESP_LOGI(TAG, "RECEIVE seq=%" PRIu32 " uptime=%" PRIu32 "s RSSI=%d dBm SNR=%d dB",
				 sequence, uptime, frame.rssi_dbm, frame.snr_db);
		ESP_ERROR_CHECK(display_show_rx(sequence));
	}
}
