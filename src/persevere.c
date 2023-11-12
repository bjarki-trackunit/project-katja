#include <katja/persevere.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/charger.h>

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

static const struct gpio_dt_spec battery_measure_enable =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), battery_measure_enable_gpios);

static const struct adc_dt_spec battery_measure_io_channel =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static void persevere_init(void)
{
	gpio_pin_configure_dt(&battery_measure_enable, GPIO_OUTPUT_ACTIVE);
	adc_channel_setup_dt(&battery_measure_io_channel);
}

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
	uint16_t sequence_buffer;
	struct adc_sequence sequence = {
		.buffer = &sequence_buffer,
		.buffer_size = sizeof(sequence_buffer),
	};
	int32_t mv;

	if (adc_sequence_init_dt(&battery_measure_io_channel, &sequence)) {
		return -EAGAIN;
	}
	if (adc_read_dt(&battery_measure_io_channel, &sequence)) {
		return -EAGAIN;
	}

	mv = (int32_t)sequence_buffer;
	adc_raw_to_millivolts_dt(&battery_measure_io_channel, &mv);
	if (mv < 0) {
		return -EAGAIN;
	}

	message->battery_voltage = (112328 * mv) / 65536;
	return 0;
}



static void persevere_routine(void)
{
	int ret;
	struct persevere_message message;

	persevere_init();

	while (true) {
		ret = persevere_get_charger_status(&message);
		if (ret < 0) {
			continue;
		}

		ret = persevere_get_battery_voltage(&message);
		if (ret < 0) {
			continue;
		}

		zbus_chan_pub(&persevere_chan, &message, K_SECONDS(1));
		k_msleep(1000);

		printk("Battery voltage %u.%03u\n", ret / 1000, ret % 1000);
	}
}

K_THREAD_DEFINE(persevere_thread, CONFIG_MAIN_STACK_SIZE, persevere_routine, NULL, NULL, NULL,
		3, 0, 0);
