#include <katja/epilogue.h>
#include <katja/conserve.h>
#include <katja/persevere.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/counter.h>

#include <zephyr/dt-bindings/power/atmel_sam_supc.h>
#include <soc.h>

#define EPILOGUE_MINIMUM_UPTIME_MS    (500)
#define EPILOGUE_MOMENT_MS            (1500)
#define EPILOGUE_MIN_BATTERY_CAPACITY (30)

static const struct device *counter = DEVICE_DT_GET(DT_ALIAS(wakeup_counter));

static void epilogue_counter_alarm_callback(const struct device *dev, uint8_t chan_id,
					    uint32_t ticks, void *user_data)
{

}

static void epilogue_set_counter_alarm(uint32_t period_ms)
{
	struct counter_alarm_cfg config = {
		.callback = epilogue_counter_alarm_callback,
		.ticks = counter_us_to_ticks(counter, (period_ms * 1000)),
		.user_data = NULL,
		.flags = 0,
	};

	counter_start(counter);
	counter_set_channel_alarm(counter, 0, &config);
}

static void epilogue_cancel_counter_alarm(void)
{
	counter_cancel_channel_alarm(counter, 0);
}

static bool epilogue_battery_capacity_is_low(void)
{
	const struct persevere_message *message;

	message = zbus_chan_const_msg(&persevere_channel);
	return message->battery_capacity < EPILOGUE_MIN_BATTERY_CAPACITY;
}

static void epilogue_await_charger(void)
{
	soc_supc_enable_wakeup_pin_source(1, SOC_SUPC_PIN_WKUP_TYPE_LOW);
}

static void epilogue_await_movement(void)
{
	soc_supc_enable_wakeup_pin_source(14, SOC_SUPC_PIN_WKUP_TYPE_HIGH);
}

static void epilogue_ignore_movement(void)
{
	soc_supc_disable_wakeup_pin_source(14);
}

static void epilogue_await_moment(void)
{
	soc_supc_enable_wakeup_source(SUPC_WAKEUP_SOURCE_RTT);
	epilogue_set_counter_alarm(EPILOGUE_MOMENT_MS);
}

static void epilogue_ignore_moment(void)
{
	epilogue_cancel_counter_alarm();
}

static void epilogue_callback(const struct zbus_channel *channel)
{
	const struct epilogue_message *message;
	uint32_t uptime_ms;

	if (channel != &epilogue_channel) {
		return;
	}

	/* Prevent immediate sleep as this prevents connecting a debugger */
	uptime_ms = k_uptime_get();
	if (uptime_ms < EPILOGUE_MINIMUM_UPTIME_MS) {
		k_msleep(EPILOGUE_MINIMUM_UPTIME_MS - uptime_ms);
	}

	message = zbus_chan_const_msg(channel);

	/* Always wake up if charger is connected */
	epilogue_await_charger();

	if (epilogue_battery_capacity_is_low()) {
		/* Prevent wakeup until charger is connected to protect battery */
		epilogue_ignore_movement();
		epilogue_ignore_moment();
		sys_poweroff();
	}

	if (message->await & EPILOGUE_AWAIT_MOVEMENT) {
		epilogue_await_movement();
	} else {
		epilogue_ignore_movement();
	}

	if (message->await & EPILOGUE_AWAIT_MOMENT) {
		epilogue_await_moment();
	} else {
		epilogue_ignore_moment();
	}

	sys_poweroff();
}

ZBUS_LISTENER_DEFINE(epilogue_listener, epilogue_callback);
