#include <katja/epilogue.h>
#include <katja/agility.h>
#include <katja/persevere.h>
#include <katja/illuminate.h>
#include <katja/rhythmic.h>
#include <katja/conserve.h>
#include <katja/connection.h>

ZBUS_CHAN_DEFINE(
	persevere_channel,
	struct persevere_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS(illuminate_subscriber, connection_subscriber),
	ZBUS_MSG_INIT(
		.charger_status = PERSEVERE_CHARGER_STATUS_DISCONNECTED,
		.battery_voltage = 0,
		.battery_capacity = 0,
	)
);

ZBUS_CHAN_DEFINE(
	agility_channel,
	struct agility_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS(illuminate_subscriber),
	ZBUS_MSG_INIT(
		.status = AGILITY_STATUS_STILL,
	)
);

ZBUS_CHAN_DEFINE(
	rhythmic_channel,
	struct rhythmic_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS(illuminate_subscriber),
	ZBUS_MSG_INIT(
		.beat = 0,
	)
);

ZBUS_CHAN_DEFINE(
	epilogue_channel,
	struct epilogue_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS(epilogue_listener),
	ZBUS_MSG_INIT()
);

ZBUS_CHAN_DEFINE(
	conserve_channel,
	struct conserve_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS(conserve_subscriber),
	ZBUS_MSG_INIT()
);

ZBUS_CHAN_DEFINE(
	connection_channel,
	struct connection_message,
	NULL,
	NULL,
	ZBUS_OBSERVERS(proxy_listener),
	ZBUS_MSG_INIT()
);
