#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LORA_PACKET_MAX_LEN 32

typedef struct {
	uint32_t frequency_hz;
	uint8_t bandwidth;
	uint8_t spreading_factor;
	uint8_t coding_rate;
	uint8_t preamble_length;
	uint8_t tx_power_dbm;
	uint8_t packet_max_len;
} lora_radio_config_t;

extern const lora_radio_config_t g_lora_radio;
extern const bool g_display_enabled;
extern const uint32_t g_lora_tx_interval_ms;
extern const uint32_t g_lora_tx_timeout_ms;
