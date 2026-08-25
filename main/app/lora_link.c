#include "lora_link.h"

#include "esp_check.h"
#include "esp_log.h"
#include "lora_hal.h"
#include "sx1262.h"
#include <inttypes.h>

#define TAG "lora_link"

// SX1262 TX timeout is expressed in 15.625 us units.
#define LORA_TX_TIMEOUT_UNITS(ms) ((uint32_t)((ms)*64U))

esp_err_t lora_link_init(void)
{
	ESP_RETURN_ON_ERROR(lora_hal_init(), TAG, "radio HAL setup failed");
	return sx1262_configure(&g_lora_radio);
}

esp_err_t lora_link_send(const uint8_t *payload, uint8_t length)
{
	esp_err_t result;
	uint16_t irq_status;
	uint8_t status;

	lora_hal_fem_set_tx();
	lora_hal_clear_event();

	result = sx1262_set_irq_mask(SX1262_IRQ_TX_EVENTS);
	if (result != ESP_OK) {
		goto restore_rx;
	}
	result = sx1262_set_packet_length(length);
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "packet config failed: %s", esp_err_to_name(result));
		goto restore_rx;
	}
	result = sx1262_write_packet(payload, length);
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "write failed: %s", esp_err_to_name(result));
		goto restore_rx;
	}
	result = sx1262_clear_irq();
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "clear IRQ failed: %s", esp_err_to_name(result));
		goto restore_rx;
	}
	result = sx1262_start_tx(LORA_TX_TIMEOUT_UNITS(1000));
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "TX start failed: %s", esp_err_to_name(result));
		goto restore_rx;
	}
	if (sx1262_get_status(&status) != ESP_OK) {
		ESP_LOGE(TAG, "radio status read failed after TX start");
	} else {
		ESP_LOGD(TAG, "SX1262 status after TX start: 0x%02x", status);
	}
	if (!lora_hal_wait_event(pdMS_TO_TICKS(g_lora_tx_timeout_ms))) {
		result = ESP_ERR_TIMEOUT;
		if (sx1262_get_irq_status(&irq_status) == ESP_OK) {
			ESP_LOGE(TAG, "TX event timeout: DIO1=%d ISR count=%" PRIu32 " IRQ=0x%04" PRIx16,
					 lora_hal_dio1_level(), lora_hal_dio1_isr_count(), irq_status);
		} else {
			ESP_LOGE(TAG, "TX event timeout: DIO1=%d ISR count=%" PRIu32 " IRQ read failed",
					 lora_hal_dio1_level(), lora_hal_dio1_isr_count());
		}
		goto restore_rx;
	}
	result = sx1262_get_irq_status(&irq_status);
	if (result != ESP_OK) {
		goto restore_rx;
	}
	result = sx1262_clear_irq();
	if ((irq_status & SX1262_IRQ_TX_DONE) == 0) {
		result = ESP_ERR_TIMEOUT;
	}

restore_rx:
	lora_hal_fem_set_rx();
	return result;
}

esp_err_t lora_link_receive(lora_frame_t *frame, TickType_t timeout)
{
	uint16_t irq_status;
	uint8_t length;
	uint8_t offset;

	lora_hal_fem_set_rx();
	lora_hal_clear_event();
	ESP_RETURN_ON_ERROR(sx1262_set_irq_mask(SX1262_IRQ_RX_EVENTS), TAG, "RX IRQ setup failed");
	ESP_RETURN_ON_ERROR(sx1262_clear_irq(), TAG, "clear IRQ failed");
	ESP_RETURN_ON_ERROR(sx1262_start_rx_continuous(), TAG, "RX start failed");
	if (!lora_hal_wait_event(timeout)) {
		return ESP_ERR_TIMEOUT;
	}
	ESP_RETURN_ON_ERROR(sx1262_get_irq_status(&irq_status), TAG, "IRQ status failed");
	ESP_RETURN_ON_ERROR(sx1262_clear_irq(), TAG, "clear IRQ failed");
	if ((irq_status & SX1262_IRQ_CRC_ERROR) != 0) {
		return ESP_ERR_INVALID_CRC;
	}
	if ((irq_status & SX1262_IRQ_RX_DONE) == 0) {
		return ESP_ERR_TIMEOUT;
	}
	ESP_RETURN_ON_ERROR(sx1262_get_rx_buffer_status(&length, &offset), TAG, "buffer status failed");
	if (length > LORA_PACKET_MAX_LEN) {
		return ESP_ERR_INVALID_SIZE;
	}
	ESP_RETURN_ON_ERROR(sx1262_get_packet_status(&frame->rssi_dbm, &frame->snr_db), TAG, "packet status failed");
	ESP_RETURN_ON_ERROR(sx1262_read_buffer(offset, frame->data, length), TAG, "packet read failed");
	frame->length = length;
	return sx1262_clear_irq();
}
