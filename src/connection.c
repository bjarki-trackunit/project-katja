#include <katja/connection.h>
#include <katja/persevere.h>
#include <katja/conserve.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/sys/atomic.h>

#define CONNECTION_IMPETUS_CHARGER_CONNECTED_BIT (0)

ZBUS_SUBSCRIBER_DEFINE(connection_subscriber, 4);

static const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));
static atomic_t connection_impetuses = ATOMIC_INIT(0);
static struct conserve_message conserve_message;
static struct net_mgmt_event_callback connection_net_callback;
static bool modem_is_resumed;

static void connection_net_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					 struct net_if *iface)
{
	static struct connection_message message;

	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		message.cellular_status = CONNECTION_STATUS_CONNECTED;
		zbus_chan_pub(&connection_channel, &message, K_SECONDS(1));
	} else if (mgmt_event == NET_EVENT_L4_DISCONNECTED){
		message.cellular_status = CONNECTION_STATUS_DISCONNECTED;
		zbus_chan_pub(&connection_channel, &message, K_SECONDS(1));
	}
}

static void connection_handle_persevere_message(const struct zbus_channel *channel)
{
	struct persevere_message message;

	if (zbus_chan_read(channel, &message, K_MSEC(500))) {
		return;
	}

	if (message.charger_status != PERSEVERE_CHARGER_STATUS_DISCONNECTED) {
		atomic_set_bit(&connection_impetuses,
			       CONNECTION_IMPETUS_CHARGER_CONNECTED_BIT);
	} else {
		atomic_clear_bit(&connection_impetuses,
				 CONNECTION_IMPETUS_CHARGER_CONNECTED_BIT);
	}
}

static void connection_connect(void)
{
	if (modem_is_resumed) {
		return;
	}

	if (pm_device_runtime_get(modem) < 0) {
		return;
	}

	if (net_if_up(net_if_get_default()) < 0) {
		pm_device_runtime_put(modem);
		return;
	}

	conserve_message.event = CONSERVE_EVENT_REFERENCE;
	zbus_chan_pub(&conserve_channel, &conserve_message, K_SECONDS(1));
	modem_is_resumed = true;
}

static void connection_disconnect(void)
{
	if (!modem_is_resumed) {
		return;
	}

	if (net_if_down(net_if_get_default()) < 0) {
		return;
	}

	if (pm_device_runtime_put(modem) < 0) {
		return;
	}

	conserve_message.event = CONSERVE_EVENT_DEREFERENCE;
	zbus_chan_pub(&conserve_channel, &conserve_message, K_SECONDS(1));
	modem_is_resumed = false;
}

static void connection_init_net_callback(void)
{
	net_mgmt_init_event_callback(&connection_net_callback, connection_net_event_handler,
				     (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED));
	net_mgmt_add_event_callback(&connection_net_callback);
}

static void connection_routine(void)
{
	const struct zbus_channel *channel;

	connection_init_net_callback();

	while (zbus_sub_wait(&connection_subscriber, &channel, K_FOREVER) == 0) {
		if (channel == &persevere_channel) {
			connection_handle_persevere_message(channel);
		}

		if (atomic_get(&connection_impetuses)) {
			connection_connect();
		} else {
			connection_disconnect();
		}
	}
}

K_THREAD_DEFINE(connection_thread, CONFIG_MAIN_STACK_SIZE, connection_routine, NULL, NULL, NULL,
		3, 0, 0);
