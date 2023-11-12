#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/pm/device_runtime.h>

#include <katja/ambience.h>
#include <katja/illuminate.h>
#include <katja/persevere.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/dt-bindings/power/atmel_sam_supc.h>

#include <soc.h>

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(led_strip));

struct led_rgb rgbs0[3] = {
};

struct led_rgb rgbs1[3] = {
	{
		.r = 255,
	},
	{
		.g = 255,
	},
	{
		.b = 255,
	},
};

struct led_rgb rgbs2[3] = {
	{
		.b = 255,
	},
	{
		.r = 255,
	},
	{
		.g = 255,
	},
};

struct led_rgb rgbs3[3] = {
	{
		.g = 255,
	},
	{
		.b = 255,
	},
	{
		.r = 255,
	},
};

struct led_rgb rgbs4[3] = {
	{
		.r = 255,
		.g = 255,
		.b = 255,
	},
	{
		.r = 255,
		.g = 255,
		.b = 255,
	},
	{
		.r = 255,
		.g = 255,
		.b = 255,
	},
};

int main(void)
{
	illuminate_resume();

	while (1) {
		led_strip_update_rgb(leds, rgbs1, ARRAY_SIZE(rgbs1));
		k_msleep(100);
		led_strip_update_rgb(leds, rgbs2, ARRAY_SIZE(rgbs2));
		k_msleep(100);
		led_strip_update_rgb(leds, rgbs3, ARRAY_SIZE(rgbs3));
		k_msleep(100);
		led_strip_update_rgb(leds, rgbs4, ARRAY_SIZE(rgbs4));
		k_msleep(500);
	}

	illuminate_suspend();

	k_msleep(10000);
	printk("Alive!!!!!!\n");
	enum ambience amb;
	volatile int ret = ambience_get(&amb);

	/* Set RTC alarm */
	struct rtc_time alarm_time = {
		.tm_sec = 0,
	};

	k_msleep(10000);

	/*
	ret = rtc_alarm_set_time(rtc, 0, (RTC_ALARM_TIME_MASK_SECOND), &alarm_time);
	soc_supc_enable_wakeup_source(SUPC_WAKEUP_SOURCE_RTC);
	printk("Sleep!!!!!!\r\n");
	sys_poweroff();
	*/
	return 0;
}
