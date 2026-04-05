#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#define RS485_RX_BUF_SIZE 256

typedef struct {
    uint8_t data[RS485_RX_BUF_SIZE];
    size_t  len;
} rs485_packet_t;

typedef struct {
    const struct device    *uart;
    struct gpio_dt_spec     dir;
    struct gpio_dt_spec     tx_led;
    struct gpio_dt_spec     rx_led;
    struct k_msgq          *rx_queue;
    int32_t                 rx_idle_timeout_us;
    /* internal — do not access directly */
    uint8_t                 _rx_buf[2][RS485_RX_BUF_SIZE];
    uint8_t                 _rx_buf_idx;
} rs485_ctx_t;

/**
 * Initialise RS485 hardware and start async receive.
 *
 * Configures the UART, direction GPIO, and LEDs.  Sets up a UART async
 * callback that copies incoming packets into ctx->rx_queue.
 */
int rs485_init(rs485_ctx_t *ctx);

int rs485_send(rs485_ctx_t *ctx, const uint8_t *data, size_t len);

int rs485_set_tx(struct gpio_dt_spec *gpio);
int rs485_set_rx(struct gpio_dt_spec *gpio);

#ifdef __cplusplus
}
#endif
