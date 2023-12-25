#ifndef KATJA_CONSERVE_H
#define KATJA_CONSERVE_H

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

enum conserve_event {
	/* Module active */
	CONSERVE_EVENT_REFERENCE = 0,
	/* Module deactivated */
	CONSERVE_EVENT_DEREFERENCE,
};

struct conserve_message {
	enum conserve_event event;
};

ZBUS_CHAN_DECLARE(conserve_channel);
ZBUS_OBS_DECLARE(conserve_listener);

static inline void conserve_publish_reference(void)
{
	struct conserve_message message = {
		.event = CONSERVE_EVENT_REFERENCE
	};

	zbus_chan_pub(&conserve_channel, &message, K_MSEC(500));
}

static inline void conserve_publish_dereference(void)
{
	struct conserve_message message = {
		.event = CONSERVE_EVENT_DEREFERENCE
	};

	zbus_chan_pub(&conserve_channel, &message, K_MSEC(500));
}

#endif /* KATJA_CONSERVE_H */
