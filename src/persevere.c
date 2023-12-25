#include <katja/persevere.h>
#include <katja/conserve.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/sensor.h>

#define PERSEVERE_BATTERY_MAX_MV (4150)
#define PERSEVERE_BATTERY_MIN_MV (3600)
#define PERSEVERE_BATTERY_MV_RANGE (PERSEVERE_BATTERY_MAX_MV - PERSEVERE_BATTERY_MIN_MV)
#define PERSEVERE_BATTERY_MV_PER_PERCENT (PERSEVERE_BATTERY_MV_RANGE / 100)

static const struct device *charger = DEVICE_DT_GET(DT_ALIAS(charger));
static const struct device *sensor = DEVICE_DT_GET(DT_ALIAS(battery_voltage_sensor));
static bool conserve_referenced;

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
	case CHARGER_STATUS_NOT_CHARGING:
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

	if (message->charger_status == PERSEVERE_CHARGER_STATUS_CHARGING) {
		charger_charge_enable(charger, false);
		k_msleep(10);
	}

	if (sensor_sample_fetch(sensor) < 0) {
		return -1;
	}

	if (message->charger_status == PERSEVERE_CHARGER_STATUS_CHARGING) {
		charger_charge_enable(charger, true);
	}

	if (sensor_channel_get(sensor, SENSOR_CHAN_VOLTAGE, &val) < 0) {
		return -1;
	}

	message->battery_voltage = (val.val1 * 1000) + (val.val2 / 1000);

	if (message->battery_voltage < 2800) {
		message->battery_voltage = 0;
	}

	return 0;
}

static void persevere_get_battery_capacity(struct persevere_message *message)
{
	if (message->battery_voltage < PERSEVERE_BATTERY_MIN_MV) {
		message->battery_capacity = 0;
		return;
	}

	message->battery_capacity = (message->battery_voltage - PERSEVERE_BATTERY_MIN_MV) /
				    (PERSEVERE_BATTERY_MV_PER_PERCENT);

	if (message->battery_capacity > 100) {
		message->battery_capacity = 100;
	}
}

static bool persevere_charger_is_connected(const struct persevere_message *message)
{
	return message->charger_status != PERSEVERE_CHARGER_STATUS_DISCONNECTED;
}

static void persevere_reference_conserve(void)
{
	if (conserve_referenced) {
		return;
	}

	conserve_publish_reference();
	conserve_referenced = true;
}

static void persevere_dereference_conserve(void)
{
	if (!conserve_referenced) {
		return;
	}

	conserve_publish_dereference();
	conserve_referenced = false;
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

		if (persevere_charger_is_connected(&message)) {
			persevere_reference_conserve();
		} else {
			persevere_dereference_conserve();
		}

		ret = persevere_get_battery_voltage(&message);
		if (ret < 0) {
			continue;
		}

		persevere_get_battery_capacity(&message);

		zbus_chan_pub(&persevere_channel, &message, K_SECONDS(1));
		k_msleep(1000);
	}
}

K_THREAD_DEFINE(persevere_thread, CONFIG_MAIN_STACK_SIZE, persevere_routine, NULL, NULL, NULL,
		3, 0, 0);
