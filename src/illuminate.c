#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec out1 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), out1_gpios);

int illuminate_resume(void)
{
	gpio_pin_configure_dt(&out1, GPIO_OUTPUT_ACTIVE);
	k_msleep(2);
	gpio_pin_configure_dt(&out1, GPIO_INPUT);
	return gpio_pin_get_dt(&out1) == 1 ? 0 : -EAGAIN;
}

int illuminate_suspend(void)
{
	gpio_pin_configure_dt(&out1, GPIO_OUTPUT_INACTIVE);
	k_msleep(2);
	gpio_pin_configure_dt(&out1, GPIO_INPUT);
	return gpio_pin_get_dt(&out1) == 0 ? 0 : -EAGAIN;
}
