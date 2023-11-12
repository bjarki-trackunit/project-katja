#ifndef KATJA_CONSERVE_H
#define KATJA_CONSERVE_H

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

enum persevere_charger_status {
	CONSERVE_CHARGER_STATUS_DISCONNECTED = 0,
	CONSERVE_CHARGER_STATUS_CHARGING,
	CONSERVE_CHARGER_STATUS_CHARGED,
};

struct persevere_message {
	/* Status of charger */
	enum persevere_charger_status charger_status;
	/* Battery voltage in millivolts */
	uint16_t battery_voltage;
	/* Battery capacity percentage */
	uint16_t battery_capacity;
};

ZBUS_CHAN_DECLARE(conserve_chan);

#endif /* KATJA_CONSERVE_H */
