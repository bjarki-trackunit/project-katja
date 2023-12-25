#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/led.h>
#include <katja/persevere.h>
#include <katja/agility.h>
#include <katja/conserve.h>
#include <katja/rhythmic.h>

#define ILLUMINATE_COLOR_MIN (10)
#define ILLUMINATE_PIXELS_SIZE (DT_PROP(DT_NODELABEL(led_strip), chain_length))

enum illuminate_shift_mode {
	ILLUMINATE_SHIFT_MODE_LTR = 0,
	ILLUMINATE_SHIFT_MODE_RTL,
	ILLUMINATE_SHIFT_MODE_SET,
	ILLUMINATE_SHIFT_MODE_EDGES,
	ILLUMINATE_SHIFT_MODE_CENTER,
};

enum illuminate_state {
	ILLUMINATE_STATE_DISCONNECTED = 0,
	ILLUMINATE_STATE_CHARGING,
	ILLUMINATE_STATE_CHARGED,
	ILLUMINATE_STATE_DISCONNECTING,
};

ZBUS_SUBSCRIBER_DEFINE(illuminate_subscriber, 4);

static const struct device *led_strip = DEVICE_DT_GET(DT_NODELABEL(led_strip));
static const struct gpio_dt_spec out1 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), out1_gpios);
static struct led_rgb pixels[ILLUMINATE_PIXELS_SIZE];
static bool resumed;
static enum illuminate_state state = ILLUMINATE_STATE_DISCONNECTED;
static enum illuminate_state next_state = ILLUMINATE_STATE_DISCONNECTED;
static size_t beat = 0;
static size_t battery_capacity = 0;
static size_t charged_pixels_size = 0;
static bool conserve_referenced;

static void illuminate_resume(void)
{
	if (resumed) {
		return;
	}

	gpio_pin_configure_dt(&out1, GPIO_OUTPUT_ACTIVE);
	k_msleep(2);
	gpio_pin_configure_dt(&out1, GPIO_INPUT);
	resumed = true;
}

static void illuminate_suspend(void)
{
	if (!resumed) {
		return;
	}

	(void)gpio_pin_configure_dt(&out1, GPIO_OUTPUT_INACTIVE);
	resumed = false;
}

static bool illuminate_any_pixel_illuminating(void)
{
	for (size_t i = 0; i < ILLUMINATE_PIXELS_SIZE; i++) {
		if ((pixels[i].r > ILLUMINATE_COLOR_MIN) ||
		    (pixels[i].g > ILLUMINATE_COLOR_MIN) ||
		    (pixels[i].b > ILLUMINATE_COLOR_MIN))
		{
			return true;
		}
	}

	return false;
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


static void illuminate_shift_pixels_ltr_range(const struct led_rgb *pixel, size_t start, size_t stop)
{
	if (stop > ILLUMINATE_PIXELS_SIZE) {
		stop = ILLUMINATE_PIXELS_SIZE;
	}

	if (stop <= start) {
		return;
	}

	for (size_t i = (stop - 1); i > start; i--) {
		pixels[i].r = pixels[i - 1].r;
		pixels[i].g = pixels[i - 1].g;
		pixels[i].b = pixels[i - 1].b;
	}

	pixels[start].r = pixel->r;
	pixels[start].g = pixel->g;
	pixels[start].b = pixel->b;
}

static inline void illuminate_shift_pixels_ltr(const struct led_rgb *pixel)
{
	illuminate_shift_pixels_ltr_range(pixel, 0, ILLUMINATE_PIXELS_SIZE);
}

static void illuminate_shift_pixels_rtl_range(const struct led_rgb *pixel, size_t start, size_t stop)
{
	if (stop > ILLUMINATE_PIXELS_SIZE) {
		stop = ILLUMINATE_PIXELS_SIZE;
	}

	if (stop <= start) {
		return;
	}

	for (size_t i = (ILLUMINATE_PIXELS_SIZE - stop); i < (ILLUMINATE_PIXELS_SIZE - 1 - start); i++) {
		pixels[i].r = pixels[i + 1].r;
		pixels[i].g = pixels[i + 1].g;
		pixels[i].b = pixels[i + 1].b;
	}

	pixels[ILLUMINATE_PIXELS_SIZE - 1 - start].r = pixel->r;
	pixels[ILLUMINATE_PIXELS_SIZE - 1 - start].g = pixel->g;
	pixels[ILLUMINATE_PIXELS_SIZE - 1 - start].b = pixel->b;
}

static inline void illuminate_shift_pixels_rtl(const struct led_rgb *pixel)
{
	illuminate_shift_pixels_rtl_range(pixel, 0, ILLUMINATE_PIXELS_SIZE);
}

static void illuminate_shift_pixels_set(const struct led_rgb *pixel)
{
	for (size_t i = 0; i < ILLUMINATE_PIXELS_SIZE; i++) {
		pixels[i].r = pixel->r;
		pixels[i].g = pixel->g;
		pixels[i].b = pixel->b;
	}
}

static inline bool illuminate_pixels_are_even(void)
{
	return (ILLUMINATE_PIXELS_SIZE % 2) == 0;
}

static void illuminate_shift_pixels_edges(const struct led_rgb *pixel)
{
	size_t halfsize = illuminate_pixels_are_even()
			? ILLUMINATE_PIXELS_SIZE / 2
			: (ILLUMINATE_PIXELS_SIZE / 2) + 1;

	illuminate_shift_pixels_rtl_range(pixel, 0, halfsize);
	illuminate_shift_pixels_ltr_range(pixel, 0, halfsize);
}

static void illuminate_shift_pixels_center(const struct led_rgb *pixel)
{
	size_t halfsize = ILLUMINATE_PIXELS_SIZE / 2;

	illuminate_shift_pixels_rtl_range(pixel, halfsize, ILLUMINATE_PIXELS_SIZE);
	illuminate_shift_pixels_ltr_range(pixel, halfsize, ILLUMINATE_PIXELS_SIZE);
}

static void illuminate_update_pixels(void)
{
	led_strip_update_rgb(led_strip, pixels, ARRAY_SIZE(pixels));
}

static size_t illuminate_pixel_size_by_percent(size_t percent)
{
	if (percent == 0) {
		return 0;
	}

	if (percent > 100) {
		return ILLUMINATE_PIXELS_SIZE;
	}

	return percent / (100 / ILLUMINATE_PIXELS_SIZE);
}

static void illuminate_disconnected_next(void)
{

}

static void illuminate_charging_next(void)
{
	charged_pixels_size = illuminate_pixel_size_by_percent(battery_capacity);
}

static void illuminate_charged_next(void)
{
	charged_pixels_size = ILLUMINATE_PIXELS_SIZE;
}

static void illuminate_disconnecting_next(void)
{

}

static void illuminate_next(void)
{
	state = next_state;
	beat = 0;

	switch (state) {
	case ILLUMINATE_STATE_DISCONNECTED:
		illuminate_disconnected_next();
		break;

	case ILLUMINATE_STATE_CHARGING:
		illuminate_charging_next();
		break;

	case ILLUMINATE_STATE_CHARGED:
		illuminate_charged_next();
		break;

	case ILLUMINATE_STATE_DISCONNECTING:
		illuminate_disconnecting_next();
		break;
	}
}

/*
static const struct led_rgb disconnected_warmup_wave_pixels[] = {
	{.r = 10, .g = 5},
	{.r = 15, .g = 7},
	{.r = 20, .g = 10},
	{.r = 25, .g = 13},
	{.r = 30, .g = 15},
	{.r = 35, .g = 17},
	{.r = 40, .g = 20},
	{.r = 45, .g = 23},
	{.r = 50, .g = 25},
};
*/

/*
static const struct led_rgb disconnected_pulse_wave_pixels[] = {
	{.r = 255, .g = 127},
	{.r = 205, .g = 103},
	{.r = 155, .g = 78},
	{.r = 105, .g = 53},
	{.r = 55, .g = 28},
};
*/

static const struct led_rgb disconnected_warmup_wave_pixels[] = {
	{.b = 10, .g = 10},
	{.b = 15, .g = 15},
	{.b = 20, .g = 20},
	{.b = 25, .g = 25},
	{.b = 30, .g = 30},
	{.b = 35, .g = 35},
	{.b = 40, .g = 40},
	{.b = 45, .g = 45},
	{.b = 50, .g = 50},
};

static const struct led_rgb disconnected_pulse_wave_pixels[] = {
	{.r = 255, .b = 127},
	{.r = 205, .b = 103},
	{.r = 155, .b = 78},
	{.r = 105, .b = 53},
	{.r = 55, .b = 28},
};

static const struct led_rgb disconnected_wave_pixel_off;

static void illuminate_disconnected_beat(void)
{
	const struct led_rgb *pixel;
	const size_t warmup_size = ARRAY_SIZE(disconnected_warmup_wave_pixels);
	const size_t pulse_size = ARRAY_SIZE(disconnected_pulse_wave_pixels);
	const size_t complete_size = warmup_size + pulse_size;

	if (beat < warmup_size) {
		pixel = &disconnected_warmup_wave_pixels[beat];
		illuminate_shift_pixels_set(pixel);
	} else if ((beat >= warmup_size) && (beat < complete_size)) {
		pixel = &disconnected_pulse_wave_pixels[beat - warmup_size];
		illuminate_shift_pixels_edges(pixel);
	} else {
		illuminate_shift_pixels_edges(&disconnected_wave_pixel_off);
	}
}

static const struct led_rgb charging_wave_pixels[] = {
	{.r = 10, .g = 40},
	{.r = 15, .g = 60},
	{.r = 30, .g = 115},
	{.r = 40, .g = 149},
	{.r = 45, .g = 180},
	{.r = 51, .g = 206},
	{.r = 55, .g = 227},
	{.r = 60, .g = 242},
	{.r = 62, .g = 251},
	{.r = 65, .g = 255},
	{.r = 65, .g = 255},
	{.r = 62, .g = 251},
	{.r = 60, .g = 242},
	{.r = 55, .g = 227},
	{.r = 51, .g = 206},
	{.r = 45, .g = 180},
	{.r = 40, .g = 149},
	{.r = 30, .g = 115},
	{.r = 15, .g = 60},
	{.r = 10, .g = 40},
};

static void illuminate_charging_beat(void)
{
	const struct led_rgb *pixel;

	if (beat < ARRAY_SIZE(charging_wave_pixels)) {
		pixel = &charging_wave_pixels[beat];
	} else {
		pixel = &charging_wave_pixels[ARRAY_SIZE(charging_wave_pixels) - 1];
	}

	illuminate_shift_pixels_ltr_range(pixel, 0, charged_pixels_size);
}

static void illuminate_charged_beat(void)
{
	const struct led_rgb *pixel;

	if (beat < ARRAY_SIZE(charging_wave_pixels)) {
		pixel = &charging_wave_pixels[beat];
	} else {
		pixel = &charging_wave_pixels[ARRAY_SIZE(charging_wave_pixels) - 1];
	}

	illuminate_shift_pixels_center(pixel);
}

static const struct led_rgb disconnecting_wave_pixels[] = {
	{.r = 10, .g = 40},
	{.r = 15, .g = 60},
	{.r = 30, .g = 115},
	{.r = 40, .g = 149},
	{.r = 45, .g = 180},
	{.r = 51, .g = 206},
	{.r = 55, .g = 227},
	{.r = 60, .g = 242},
	{.r = 62, .g = 251},
	{.r = 65, .g = 255},
	{.r = 65, .g = 255},
	{.r = 62, .g = 251},
	{.r = 60, .g = 242},
	{.r = 55, .g = 227},
	{.r = 51, .g = 206},
	{.r = 45, .g = 180},
	{.r = 40, .g = 149},
	{.r = 30, .g = 115},
	{.r = 15, .g = 60},
	{.r = 10, .g = 40},
};

static const struct led_rgb disconnecting_wave_pixel_off;

static void illuminate_disconnecting_beat(void)
{
	const struct led_rgb *pixel;

	if (beat < ARRAY_SIZE(charging_wave_pixels)) {
		pixel = &disconnecting_wave_pixels[beat];
	} else {
		pixel = &disconnecting_wave_pixel_off;
	}

	illuminate_shift_pixels_rtl_range(pixel, ILLUMINATE_PIXELS_SIZE - charged_pixels_size,
					  ILLUMINATE_PIXELS_SIZE);
}

static void illuminate_beat(void)
{
	switch (state) {
	case ILLUMINATE_STATE_DISCONNECTED:
		illuminate_disconnected_beat();
		break;

	case ILLUMINATE_STATE_CHARGING:
		illuminate_charging_beat();
		break;

	case ILLUMINATE_STATE_CHARGED:
		illuminate_charged_beat();
		break;

	case ILLUMINATE_STATE_DISCONNECTING:
		illuminate_disconnecting_beat();
		break;
	}

	if (illuminate_any_pixel_illuminating()) {
		persevere_reference_conserve();
		illuminate_resume();
		illuminate_update_pixels();

	} else {
		illuminate_update_pixels();
		illuminate_suspend();
		persevere_dereference_conserve();
	}

	beat++;
}

static inline bool illuminate_beat_is_next(size_t beat)
{
	return (beat % 80) == 0;
}

static void illuminate_handle_rhythmic_message(const struct zbus_channel *channel)
{
	struct rhythmic_message message;

	if (zbus_chan_read(channel, &message, K_MSEC(250))) {
		return;
	}

	if (illuminate_beat_is_next(message.beat)) {
		illuminate_next();
	}

	illuminate_beat();
}

static void illuminate_handle_persevere_message(const struct zbus_channel *channel)
{
	struct persevere_message message;

	if (zbus_chan_read(channel, &message, K_MSEC(250))) {
		return;
	}

	battery_capacity = message.battery_capacity;

	switch (state) {
	case ILLUMINATE_STATE_DISCONNECTED:
		switch (message.charger_status) {
			case PERSEVERE_CHARGER_STATUS_CHARGING:
				next_state = ILLUMINATE_STATE_CHARGING;
				break;
			case PERSEVERE_CHARGER_STATUS_CHARGED:
				next_state = ILLUMINATE_STATE_CHARGED;
				break;
			default:
				break;
		}

		break;

	case ILLUMINATE_STATE_CHARGING:
		switch (message.charger_status) {
			case PERSEVERE_CHARGER_STATUS_CHARGED:
			case PERSEVERE_CHARGER_STATUS_DISCONNECTED:
				next_state = ILLUMINATE_STATE_DISCONNECTING;
				break;
			default:
				break;
		}

		break;

	case ILLUMINATE_STATE_CHARGED:
		switch (message.charger_status) {
			case PERSEVERE_CHARGER_STATUS_CHARGING:
			case PERSEVERE_CHARGER_STATUS_DISCONNECTED:
				next_state = ILLUMINATE_STATE_DISCONNECTING;
				break;
			default:
				break;
		}

		break;

	case ILLUMINATE_STATE_DISCONNECTING:
		switch (message.charger_status) {
			case PERSEVERE_CHARGER_STATUS_CHARGING:
				next_state = ILLUMINATE_STATE_CHARGING;
				break;
			case PERSEVERE_CHARGER_STATUS_CHARGED:
				next_state = ILLUMINATE_STATE_CHARGED;
				break;
			case PERSEVERE_CHARGER_STATUS_DISCONNECTED:
				next_state = ILLUMINATE_STATE_DISCONNECTED;
				break;
		}

		break;
	}
}

static void illuminate_routine(void)
{
	const struct zbus_channel *channel;

	while (zbus_sub_wait(&illuminate_subscriber, &channel, K_FOREVER) == 0) {
		if (channel == &persevere_channel) {
			illuminate_handle_persevere_message(channel);
		}
		if (channel == &rhythmic_channel) {
			illuminate_handle_rhythmic_message(channel);
		}
	}
}

K_THREAD_DEFINE(illuminate_thread, CONFIG_MAIN_STACK_SIZE, illuminate_routine, NULL, NULL, NULL,
		3, 0, 0);
