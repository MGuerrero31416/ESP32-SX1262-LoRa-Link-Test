#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t display_init(const char *role);
esp_err_t display_show_tx(uint32_t sequence);
esp_err_t display_show_rx(uint32_t sequence);