#ifndef KATJA_CONNECTION_H
#define KATJA_CONNECTION_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/zbus/zbus.h>

enum connection_status {
	/* Domain is disconnected */
	CONNECTION_STATUS_DISCONNECTED = 0,
	/* Domain is currently connected */
	CONNECTION_STATUS_CONNECTED,
};

struct connection_message {
	enum connection_status cellular_status;
};

ZBUS_CHAN_DECLARE(connection_channel);
ZBUS_OBS_DECLARE(connection_subscriber);

#endif /* KATJA_CONNECTION_H */
