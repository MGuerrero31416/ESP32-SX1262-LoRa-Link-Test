#include "display.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define TAG "display"
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_PAGE_COUNT 8

static i2c_master_dev_handle_t oled_device;

static esp_err_t display_command(uint8_t command)
{
	uint8_t transaction[] = {0x00, command};
	return i2c_master_transmit(oled_device, transaction, sizeof(transaction), -1);
}

static esp_err_t display_data(const uint8_t *data, size_t length)
{
	uint8_t transaction[17] = {0x40};
	while (length > 0) {
		size_t chunk_length = length > sizeof(transaction) - 1 ? sizeof(transaction) - 1 : length;
		for (size_t index = 0; index < chunk_length; index++) {
			transaction[index + 1] = data[index];
		}
		ESP_RETURN_ON_ERROR(i2c_master_transmit(oled_device, transaction, chunk_length + 1, -1), TAG, "OLED data write failed");
		data += chunk_length;
		length -= chunk_length;
	}
	return ESP_OK;
}

static esp_err_t display_clear(void)
{
	uint8_t empty[OLED_WIDTH] = {0};
	for (uint8_t page = 0; page < OLED_PAGE_COUNT; page++) {
		ESP_RETURN_ON_ERROR(display_command(0xB0 | page), TAG, "page select failed");
		ESP_RETURN_ON_ERROR(display_command(0x00), TAG, "column select failed");
		ESP_RETURN_ON_ERROR(display_command(0x10), TAG, "column select failed");
		ESP_RETURN_ON_ERROR(display_data(empty, sizeof(empty)), TAG, "clear failed");
	}
	return ESP_OK;
}

static const uint8_t *display_glyph(char character)
{
	static const uint8_t blank[] = {0, 0, 0, 0, 0};
	static const uint8_t digits[][5] = {
		{0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
		{0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
		{0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
		{0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
		{0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
	};
	static const uint8_t letters[][5] = {
		{0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x41},
		{0x7F, 0x09, 0x09, 0x09, 0x01}, {0x7F, 0x09, 0x19, 0x29, 0x46},
		{0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
		{0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
		{0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
		{0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
		{0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
		{0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
		{0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
		{0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
		{0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
	};

	if (character >= '0' && character <= '9') {
		return digits[character - '0'];
	}
	if (character >= 'A' && character <= 'Z') {
		return letters[character - 'A'];
	}
	return blank;
}

static esp_err_t display_show_packet(const char *direction, uint32_t sequence)
{
	char message[22];
	uint8_t data[OLED_WIDTH] = {0};
	int text_length = snprintf(message, sizeof(message), "%s PACKET %lu", direction, (unsigned long)sequence);
	ESP_RETURN_ON_FALSE(text_length > 0 && (size_t)text_length < sizeof(message), ESP_ERR_INVALID_SIZE, TAG, "message too long");

	for (int character_index = 0; character_index < text_length; character_index++) {
		const uint8_t *glyph = display_glyph(message[character_index]);
		for (size_t column = 0; column < 5; column++) {
			data[character_index * 6 + column] = glyph[column];
		}
	}
	ESP_RETURN_ON_ERROR(display_clear(), TAG, "clear failed");
	ESP_RETURN_ON_ERROR(display_command(0xB3), TAG, "page select failed");
	ESP_RETURN_ON_ERROR(display_command(0x00), TAG, "column select failed");
	ESP_RETURN_ON_ERROR(display_command(0x10), TAG, "column select failed");
	return display_data(data, sizeof(data));
}

esp_err_t display_init(void)
{
	const uint8_t initialization[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF};
	const gpio_config_t output = {
		.pin_bit_mask = (1ULL << OLED_VEXT) | (1ULL << OLED_RESET),
		.mode = GPIO_MODE_OUTPUT,
	};
	i2c_master_bus_handle_t i2c_bus;
	const i2c_master_bus_config_t bus_config = {
		.i2c_port = I2C_NUM_0,
		.sda_io_num = OLED_SDA,
		.scl_io_num = OLED_SCL,
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.flags.enable_internal_pullup = true,
	};
	const i2c_device_config_t device_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = OLED_ADDRESS,
		.scl_speed_hz = 400000,
	};

	ESP_RETURN_ON_ERROR(gpio_config(&output), TAG, "OLED GPIO setup failed");
	gpio_set_level(OLED_VEXT, 0);
	gpio_set_level(OLED_RESET, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	gpio_set_level(OLED_RESET, 1);
	ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &i2c_bus), TAG, "I2C bus setup failed");
	ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &device_config, &oled_device), TAG, "OLED setup failed");
	for (size_t index = 0; index < sizeof(initialization); index++) {
		ESP_RETURN_ON_ERROR(display_command(initialization[index]), TAG, "OLED initialization failed");
	}
	return display_clear();
}

esp_err_t display_show_tx(uint32_t sequence)
{
	return display_show_packet("TX", sequence);
}

esp_err_t display_show_rx(uint32_t sequence)
{
	return display_show_packet("RX", sequence);
}