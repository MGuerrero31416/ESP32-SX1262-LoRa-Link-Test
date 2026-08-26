#include "User_Settings.h"

const lora_radio_config_t g_lora_radio = {
	.frequency_hz = 923000000UL, // Heltec WiFi LoRa 32 V4 / GC1109 band
	.bandwidth = 0x04, // 125 kHz
	.spreading_factor = 10, // Robust indoor starting point; use SF11 if the link is marginal
	.coding_rate = 0x01, // 4/5
	.preamble_length = 12, // Extra acquisition margin in indoor multipath
	.tx_power_dbm = 22, // Board PA setting
	.packet_max_len = LORA_PACKET_MAX_LEN, // Maximum packet length
};

const bool g_display_enabled = true; // Enable or disable the OLED display functionality

const uint32_t g_lora_tx_interval_ms = 2000; // Interval between transmissions in milliseconds
const uint32_t g_lora_tx_timeout_ms = 5000; // Timeout for waiting for TX completion in milliseconds
