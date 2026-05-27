#include "can.h"

#include <string.h>

LOG_MODULE_REGISTER(can);

static void can_tx_done(const struct device *dev, int error, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    if (error != 0 && error != -ENETUNREACH && error != -ENETDOWN) {
        LOG_WRN("CAN TX error: %d", error);
    }
}

int can_send_(const struct device *can_dev, uint16_t id, uint8_t *data, uint8_t data_len)
{
    struct can_frame frame = {0};
    frame.id               = id;
    frame.dlc              = data_len;
    frame.flags            = CAN_FRAME_IDE;

    memcpy(frame.data, data, data_len);

    int ret = can_send(can_dev, &frame, K_MSEC(10), can_tx_done, NULL);
    if (ret != 0 && ret != -ENETUNREACH && ret != -ENETDOWN) {
        LOG_WRN("CAN send queue error: %d", ret);
    }
    return ret;
}

int can_send_float(const struct device *can_dev, uint16_t id, float value)
{
    return can_send_(can_dev, id, (uint8_t *)&value, sizeof(float));
}

int can_add_rx_filter_(const struct device *can_dev, can_rx_callback_t can_rx_callback,
                       const struct can_filter *filter)
{
    int filter_id = can_add_rx_filter(can_dev, can_rx_callback, NULL, filter);
    if (filter_id < 0) {
        LOG_ERR("Unable to add rx filter [%d]", filter_id);
    } else {
        LOG_INF("CAN rx filter [%d] has been added succesfully", filter_id);
    }
    return filter_id;
}

int can_init(const struct device *can_dev, uint32_t baudrate)
{
    struct can_timing timing;
    int               ret;
    ret = can_calc_timing(can_dev, &timing, baudrate, 875);
    if (ret > 0) {
        LOG_INF("Sample-Point error: %d", ret);
    }
    if (ret < 0) {
        LOG_ERR("Failed to calc a valid timing");
        return -1;
    }

    ret = can_set_timing(can_dev, &timing);
    if (ret != 0) {
        LOG_ERR("Failed to set timing");
    }

    ret = can_start(can_dev);
    if (ret != 0) {
        LOG_ERR("Failed to start CAN controller");
    }

    if (ret == 0) {
        LOG_INF("CAN initialized correctly");
    }

    return ret;
}
