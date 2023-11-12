#include <katja/ambience.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>

const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
static struct rtc_time datetime;

static void ambience_routine(void)
{
	int ret;

	while (true) {
		ret = rtc_get_time(rtc, &datetime);
		if (ret < 0) {
			k_msleep(10020);
			continue;
		}

		printk("Time: %02u:%02u:%02u\n", datetime.tm_hour, datetime.tm_min,
		       datetime.tm_sec);
		k_msleep(10020);
	}
}

K_THREAD_DEFINE(ambience_thread, CONFIG_MAIN_STACK_SIZE, ambience_routine, NULL, NULL, NULL,
		3, 0, 0);
