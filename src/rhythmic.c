#include <katja/rhythmic.h>
#include <zephyr/kernel.h>

static struct rhythmic_message message;

static void rhythmic_timer_expired(struct k_timer *timer_id)
{
	zbus_chan_pub(&rhythmic_channel, &message, K_MSEC(35));
	message.beat++;
}

K_TIMER_DEFINE(rhythmic_timer, rhythmic_timer_expired, NULL);

static int rhythmic_init(void)
{
	k_timer_start(&rhythmic_timer, K_NO_WAIT, K_MSEC(35));
	return 0;
}

SYS_INIT(rhythmic_init, APPLICATION, 99);
