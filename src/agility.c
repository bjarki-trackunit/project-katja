#include <katja/agility.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/pm/device_runtime.h>

#define AGILITY_MOTION_TIMEOUT (K_MSEC(500))
#define AGILITY_MOTION_FILTER  (10)

const struct device *accelerometer = DEVICE_DT_GET(DT_ALIAS(acc));
const struct device *retmem = DEVICE_DT_GET(DT_ALIAS(retmem));

ZBUS_SUBSCRIBER_DEFINE(agility_subscriber, 4);

static const struct sensor_trigger motion_trigger = {
	.type = SENSOR_TRIG_MOTION,
	.chan = SENSOR_CHAN_ACCEL_XYZ,
};

static K_SEM_DEFINE(motion, 0, 1);

static void agility_tap_handler(const struct device *dev,
				const struct sensor_trigger *trigger)
{
	k_sem_give(&motion);
}

static void agility_update_filter(bool motion_is_detected)
{
	uint8_t filter;

	if (motion_is_detected) {
		filter = AGILITY_MOTION_FILTER;
		retained_mem_write(retmem, 0, &filter, 1);
	} else {
		retained_mem_read(retmem, 0, &filter, 1);

		if (filter > AGILITY_MOTION_FILTER) {
			/* Filter not initialized properly */
			filter = AGILITY_MOTION_FILTER;
		}

		if (filter > 0) {
			filter--;
			retained_mem_write(retmem, 0, &filter, 1);
		}
	}
}

static bool agility_is_moving(void)
{
	uint8_t filter;

	retained_mem_read(retmem, 0, &filter, 1);
	return filter > 0;
}

static void agility_routine(void)
{
	int ret;
	struct agility_message message;

	message.status = agility_is_moving() ? AGILITY_STATUS_MOVING : AGILITY_STATUS_STILL;
	zbus_chan_pub(&agility_channel, &message, K_SECONDS(1));

	ret = sensor_trigger_set(accelerometer, &motion_trigger, agility_tap_handler);
	if (ret < 0) {
		return;
	}

	while(true) {
		if (k_sem_take(&motion, AGILITY_MOTION_TIMEOUT) == 0) {
			agility_update_filter(true);
		} else {
			agility_update_filter(false);
		}

		if (agility_is_moving() && message.status == AGILITY_STATUS_STILL) {
			message.status = AGILITY_STATUS_MOVING;
			zbus_chan_pub(&agility_channel, &message, K_SECONDS(1));
		} else if (!agility_is_moving() && message.status == AGILITY_STATUS_MOVING) {
			message.status = AGILITY_STATUS_STILL;
			zbus_chan_pub(&agility_channel, &message, K_SECONDS(1));
		}
	}
}

K_THREAD_DEFINE(agility_thread, CONFIG_MAIN_STACK_SIZE, agility_routine, NULL, NULL, NULL,
		3, 0, 0);
