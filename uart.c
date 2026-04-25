#include "uart.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart);

int uart_device_init(const struct device *dev) {
    if (!device_is_ready(dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }
    LOG_INF("UART device ready");
    return 0;
}

int uart_rx_init(const struct device *dev, uint8_t *buf, size_t len,
                 int32_t timeout) {
    int ret = uart_rx_enable(dev, buf, len, timeout);
    if (ret < 0) {
        LOG_ERR("UART RX enable failed: %d", ret);
    } else {
        LOG_INF("UART RX enabled");
    }
    return ret;
}

int uart_callback_register(const struct device *dev, uart_callback_t callback,
                           void *user_data) {
    int ret = uart_callback_set(dev, callback, user_data);
    if (ret < 0) {
        LOG_ERR("UART callback set failed: %d", ret);
    } else {
        LOG_INF("UART callback configured succesfully");
    }
    return ret;
}
