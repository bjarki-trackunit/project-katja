#ifndef KATJA_PERSEVERE_H
#define KATJA_PERSEVERE_H

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

enum persevere_charger_status {
	PERSEVERE_CHARGER_STATUS_DISCONNECTED = 0,
	PERSEVERE_CHARGER_STATUS_CHARGING,
	PERSEVERE_CHARGER_STATUS_CHARGED,
};

struct persevere_message {
	/* Status of charger */
	enum persevere_charger_status charger_status;
	/* Battery voltage in millivolts */
	uint16_t battery_voltage;
	/* Battery capacity percentage */
	uint16_t battery_capacity;
};

ZBUS_CHAN_DECLARE(persevere_chan);

#endif /* KATJA_PERSEVERE_H */
