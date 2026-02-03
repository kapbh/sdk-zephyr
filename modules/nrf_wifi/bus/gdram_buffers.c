/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "gdram_buffers.h"

LOG_MODULE_REGISTER(wifi_gdram_buffers, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

/**
 * @brief Initialize GDRAM buffer for WiFi ADC capture
 *
 * Clears the buffer. If memory is not accessible (MPC misconfiguration),
 * the memset will trigger a fault.
 */
int wifi_gdram_buffers_init(void)
{
	LOG_INF("Initializing WiFi ADC I/Q buffer in GDRAM (RAM_02)");
	LOG_INF("Address: 0x%08lX, Size: %u bytes",
		WIFI_ADC_IQ_BUFFER_ADDR, WIFI_ADC_IQ_BUFFER_SIZE);

	/* Clear buffer - will fault if memory not accessible */
	memset((void *)WIFI_ADC_IQ_BUFFER_ADDR, 0, WIFI_ADC_IQ_BUFFER_SIZE);

	LOG_INF("ADC I/Q buffer initialized successfully");
	return 0;
}
