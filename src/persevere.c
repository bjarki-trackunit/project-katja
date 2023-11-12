#include <katja/persevere.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/sensor.h>

ZBUS_CHAN_DEFINE(
	persevere_chan,
	struct persevere_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS_EMPTY,
	ZBUS_MSG_INIT(
		.charger_status = PERSEVERE_CHARGER_STATUS_DISCONNECTED,
		.battery_voltage = 0,
		.battery_capacity = 0,
	)
);

static const struct device *charger = DEVICE_DT_GET(DT_ALIAS(charger));
static const struct device *battery_voltage_sensor =
	DEVICE_DT_GET(DT_ALIAS(battery_voltage_sensor));

static int persevere_get_charger_status(struct persevere_message *message)
{
	union charger_propval val;

	if (charger_get_prop(charger, CHARGER_PROP_STATUS, &val) < 0) {
		return -1;
	}

	switch (val.status) {
	case CHARGER_STATUS_CHARGING:
		message->charger_status = PERSEVERE_CHARGER_STATUS_CHARGING;
		break;

	case CHARGER_STATUS_DISCHARGING:
		message->charger_status = PERSEVERE_CHARGER_STATUS_DISCONNECTED;
		break;

	case CHARGER_STATUS_FULL:
		message->charger_status = PERSEVERE_CHARGER_STATUS_CHARGED;
		break;

	default:
		return -1;
	}

	return 0;
}

static int persevere_get_battery_voltage(struct persevere_message *message)
{
	struct sensor_value val;

	if (sensor_sample_fetch(battery_voltage_sensor) < 0) {
		return -1;
	}

	if (sensor_channel_get(battery_voltage_sensor, SENSOR_CHAN_VOLTAGE, &val) < 0) {
		return -1;
	}

	message->battery_voltage = (val.val1 * 1000) + (val.val2 / 1000);
	return 0;
}

static void persevere_get_battery_capacity(struct persevere_message *message)
{
	/* Follows standard Li-Ion discharge curve */
	if (message->battery_voltage > 3900) {
		message->battery_capacity = 100;
	} else if (message->battery_voltage > 3700) {
		message->battery_capacity = 80;
	} else if (message->battery_voltage > 3650) {
		message->battery_capacity = 40;
	} else if (message->battery_voltage > 3500) {
		message->battery_capacity = 20;
	} else {
		message->battery_capacity = 0;
	}
}

static void persevere_routine(void)
{
	int ret;
	struct persevere_message message;

	while (true) {
		ret = persevere_get_charger_status(&message);
		if (ret < 0) {
			continue;
		}

		ret = persevere_get_battery_voltage(&message);
		if (ret < 0) {
			continue;
		}

		persevere_get_battery_capacity(&message);

		zbus_chan_pub(&persevere_chan, &message, K_SECONDS(1));
		k_msleep(1000);

		printk("Battery voltage %u.%03u\n", ret / 1000, ret % 1000);
	}
}

K_THREAD_DEFINE(persevere_thread, CONFIG_MAIN_STACK_SIZE, persevere_routine, NULL, NULL, NULL,
		3, 0, 0);
