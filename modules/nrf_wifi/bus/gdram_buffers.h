/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_NRF_WIFI_BUS_GDRAM_BUFFERS_H_
#define ZEPHYR_MODULES_NRF_WIFI_BUS_GDRAM_BUFFERS_H_

#include <zephyr/devicetree.h>

/**
 * @file gdram_buffers.h
 * @brief GDRAM buffer definitions for WiFi Radio Test ADC I/Q capture
 *
 * This buffer is allocated in RAM_02 (GDRAM) for direct DMA transfer
 * from the WiFi firmware to host memory.
 *
 */

/* ADC I/Q samples buffer in RAM_02 (GDRAM) */
#define WIFI_ADC_IQ_BUFFER_ADDR      0x200C2000UL
#define WIFI_ADC_IQ_BUFFER_SIZE      (32 * 1024)  /* 32KB */

/**
 * @brief Initialize GDRAM buffer for WiFi ADC capture
 *
 * @return 0 on success, negative errno on failure
 */
int wifi_gdram_buffers_init(void);

/**
 * @brief Get ADC I/Q buffer address to pass to firmware
 *
 * @return Physical address of ADC I/Q buffer in GDRAM (0x200C2000)
 */
static inline uint32_t wifi_get_adc_iq_buffer_addr(void)
{
	return WIFI_ADC_IQ_BUFFER_ADDR;
}

/**
 * @brief Get ADC I/Q buffer size
 *
 * @return Size of ADC I/Q buffer in bytes (32768)
 */
static inline uint32_t wifi_get_adc_iq_buffer_size(void)
{
	return WIFI_ADC_IQ_BUFFER_SIZE;
}

#endif /* ZEPHYR_MODULES_NRF_WIFI_BUS_GDRAM_BUFFERS_H_ */
