#include "User_Settings.h"
#include "display.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lora_link.h"
#include "lora_rx_task.h"
#include "lora_tx_task.h"
#include "sdkconfig.h"
#include <inttypes.h>

#define TAG "lora"

void app_main(void)
{
	ESP_ERROR_CHECK(display_init());
	ESP_ERROR_CHECK(lora_link_init());
#if CONFIG_LORA_ROLE_TRANSMITTER
	ESP_LOGI(TAG, "TRANSMITTER: %" PRIu32 " Hz BW125 SF%u CR4/5 CRC on",
			 g_lora_radio.frequency_hz, g_lora_radio.spreading_factor);
	lora_tx_task_run(NULL);
#else
	ESP_LOGI(TAG, "RECEIVER: %" PRIu32 " Hz BW125 SF%u CR4/5 CRC on",
			 g_lora_radio.frequency_hz, g_lora_radio.spreading_factor);
	lora_rx_task_run(NULL);
#endif
}
