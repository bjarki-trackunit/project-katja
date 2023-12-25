#ifndef KATJA_AGILITY_H
#define KATJA_AGILITY_H

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

enum agility_status {AGILITY_STATUS_STILL = 0,
	AGILITY_STATUS_MOVING,
};

struct agility_message {
	enum agility_status status;
};

ZBUS_CHAN_DECLARE(agility_channel);

#endif /* KATJA_AGILITY_H */
