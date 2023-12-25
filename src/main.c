#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>

static const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog));

int main(void)
{
	while (true) {
		wdt_feed(wdt, 0);
		k_msleep(300);
	}

	return 0;
}
