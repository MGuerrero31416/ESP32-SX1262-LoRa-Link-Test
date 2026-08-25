#pragma once

#include "User_Settings.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include <stdint.h>

typedef struct {
	uint8_t data[LORA_PACKET_MAX_LEN];
	uint8_t length;
	int16_t rssi_dbm;
	int8_t snr_db;
} lora_frame_t;

esp_err_t lora_link_init(void);
esp_err_t lora_link_send(const uint8_t *payload, uint8_t length);
esp_err_t lora_link_receive(lora_frame_t *frame, TickType_t timeout);
