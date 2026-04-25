/**
 * @file uart.h
 * @brief Thin wrappers around the Zephyr async UART API with logging.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/uart.h>

/**
 * @brief Check that a UART device is ready.
 *
 * @param dev  UART device to check.
 *
 * @retval 0        Device is ready.
 * @retval -ENODEV  Device is not ready.
 */
int uart_device_init(const struct device *dev);

/**
 * @brief Enable asynchronous UART reception with DMA.
 *
 * @param dev      UART device.
 * @param buf      Initial DMA receive buffer.
 * @param len      Size of @p buf in bytes.
 * @param timeout  Idle timeout in microseconds before UART_RX_RDY fires for
 *                 a partial buffer.  Pass @c SYS_FOREVER_US to disable.
 *
 * @return 0 on success, negative errno on failure.
 */
int uart_rx_init(const struct device *dev, uint8_t *buf, size_t len,
                 int32_t timeout);

/**
 * @brief Register an asynchronous UART event callback.
 *
 * @param dev        UART device.
 * @param callback   Event callback function.
 * @param user_data  Opaque pointer passed to @p callback on every event.
 *
 * @return 0 on success, negative errno on failure.
 */
int uart_callback_register(const struct device *dev, uart_callback_t callback,
                           void *user_data);

#ifdef __cplusplus
}
#endif
