#include "User_Settings.h"

const lora_radio_config_t g_lora_radio = {
	.frequency_hz = 923000000UL,
	.bandwidth = 0x04, // 125 kHz
	.spreading_factor = 7,
	.coding_rate = 0x01, // 4/5
	.preamble_length = 8,
	.tx_power_dbm = 10,
	.packet_max_len = LORA_PACKET_MAX_LEN,
};

const uint32_t g_lora_tx_interval_ms = 2000;
const uint32_t g_lora_tx_timeout_ms = 5000;
