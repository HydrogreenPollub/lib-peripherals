#include "rs485.h"
#include "gpio.h"
#include "uart.h"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rs485, LOG_LEVEL_INF);

static struct k_mutex tx_mutex;
static struct k_sem   tx_done_sem;

/* Single global context pointer — one RS485 bus per application. */
static rs485_ctx_t *g_ctx;

int rs485_set_rx(struct gpio_dt_spec *gpio) {
    return gpio_reset(gpio); /* 0 = RX mode */
}

int rs485_set_tx(struct gpio_dt_spec *gpio) {
    return gpio_set(gpio); /* 1 = TX mode */
}

static void rs485_uart_cb(const struct device *dev, struct uart_event *evt,
                          void *user_data) {
    ARG_UNUSED(user_data);

    rs485_ctx_t *ctx = g_ctx;
    if (!ctx) {
        return;
    }

    switch (evt->type) {
    case UART_RX_RDY: {
        rs485_packet_t pkt;
        size_t len = evt->data.rx.len;
        if (len > RS485_RX_BUF_SIZE) {
            len = RS485_RX_BUF_SIZE;
        }
        memcpy(pkt.data, evt->data.rx.buf + evt->data.rx.offset, len);
        pkt.len = len;
        k_msgq_put(ctx->rx_queue, &pkt, K_NO_WAIT);
        gpio_set(&ctx->rx_led);
        break;
    }

    case UART_RX_BUF_REQUEST: {
        uint8_t next = ctx->_rx_buf_idx ^ 1u;
        int rsp_ret = uart_rx_buf_rsp(dev, ctx->_rx_buf[next], RS485_RX_BUF_SIZE);
        if (rsp_ret != 0) {
            LOG_ERR("uart_rx_buf_rsp failed (%d) — DMA ping-pong broken", rsp_ret);
        }
        break;
    }

    case UART_RX_BUF_RELEASED:
        ctx->_rx_buf_idx ^= 1u;
        break;

    case UART_RX_DISABLED: {
        gpio_reset(&ctx->rx_led);
        int en_ret = uart_rx_enable(dev, ctx->_rx_buf[ctx->_rx_buf_idx], RS485_RX_BUF_SIZE,
                                    ctx->rx_idle_timeout_us);
        if (en_ret != 0) {
            LOG_ERR("uart_rx_enable failed (%d) — RS485 RX stopped permanently", en_ret);
        }
        break;
    }

    case UART_TX_DONE:
        rs485_set_rx(&ctx->dir);
        k_sem_give(&tx_done_sem);
        break;

    case UART_TX_ABORTED:
        rs485_set_rx(&ctx->dir);
        k_mutex_unlock(&tx_mutex);
        break;

    default:
        break;
    }
}

int rs485_init(rs485_ctx_t *ctx) {
    g_ctx = ctx;

    k_mutex_init(&tx_mutex);
    k_sem_init(&tx_done_sem, 0, 1);

    int ret = uart_device_init(ctx->uart);
    if (ret != 0) {
        LOG_ERR("RS485 UART not ready");
        return ret;
    }

    ret = gpio_init(&ctx->dir, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("RS485 dir GPIO not ready");
        return ret;
    }
    rs485_set_rx(&ctx->dir);

    ret = gpio_init(&ctx->tx_led, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("RS485 tx_led GPIO not ready");
        return ret;
    }

    ret = gpio_init(&ctx->rx_led, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("RS485 rx_led GPIO not ready");
        return ret;
    }

    ret = uart_callback_register(ctx->uart, rs485_uart_cb, NULL);
    if (ret != 0) {
        LOG_ERR("RS485 UART callback set failed");
        return ret;
    }

    ctx->_rx_buf_idx = 0;
    ret = uart_rx_enable(ctx->uart, ctx->_rx_buf[0], RS485_RX_BUF_SIZE,
                         ctx->rx_idle_timeout_us);
    if (ret != 0) {
        LOG_ERR("RS485 RX enable failed");
        return ret;
    }

    LOG_INF("RS485 initialised");
    return 0;
}

int rs485_send(rs485_ctx_t *ctx, const uint8_t *data, size_t len) {
    k_mutex_lock(&tx_mutex, K_FOREVER);
    gpio_set(&ctx->tx_led);
    rs485_set_tx(&ctx->dir);

    int ret = uart_tx(ctx->uart, data, len, SYS_FOREVER_MS);
    if (ret != 0) {
        LOG_ERR("RS485 TX failed");
        rs485_set_rx(&ctx->dir);
        gpio_reset(&ctx->tx_led);
        k_mutex_unlock(&tx_mutex);
        return ret;
    }

    k_sem_take(&tx_done_sem, K_MSEC(100));
    gpio_reset(&ctx->tx_led);
    k_mutex_unlock(&tx_mutex);
    return 0;
}
