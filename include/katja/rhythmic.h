#ifndef KATJA_RHYTHMIC_H
#define KATJA_RHYTHMIC_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/zbus/zbus.h>

struct rhythmic_message {
	/* Number of beats since wakeup */
	size_t beat;
};

ZBUS_CHAN_DECLARE(rhythmic_channel);

#endif /* KATJA_RHYTHMIC_H */
