#include <katja/conserve.h>
#include <katja/agility.h>
#include <katja/persevere.h>
#include <katja/epilogue.h>
#include <zephyr/sys/atomic.h>

ZBUS_SUBSCRIBER_DEFINE(conserve_subscriber, 4);

static atomic_t conserve_references = ATOMIC_INIT(0);

static void conserve_handle_conserve_message(const struct zbus_channel *channel)
{
	struct conserve_message message;

	if (zbus_chan_read(channel, &message, K_MSEC(500))) {
		return;
	}

	switch (message.event) {
	case CONSERVE_EVENT_REFERENCE:
		atomic_inc(&conserve_references);
		break;

	case CONSERVE_EVENT_DEREFERENCE:
		atomic_dec(&conserve_references);
		break;
	}
}

static bool conserve_uptime_exceeds_ms(uint32_t ms)
{
	return k_uptime_get() > ms;
}

static bool conserve_has_no_references(void)
{
	return atomic_get(&conserve_references) == 0;
}

static bool conserve_epilogue_is_imminent(void)
{
	if (conserve_uptime_exceeds_ms(200) &&
	    conserve_has_no_references()) {
		return true;
	}

	return false;
}

static bool conserve_is_still(void)
{
	struct agility_message message;

	if (zbus_chan_read(&agility_channel, &message, K_MSEC(500))) {
		return false;
	}

	return message.status == AGILITY_STATUS_STILL;
}

static void conserve_request_epilogue(void)
{
	struct epilogue_message message;

	if (conserve_is_still()) {
		message.await = EPILOGUE_AWAIT_MOVEMENT;
	} else {
		message.await = EPILOGUE_AWAIT_MOMENT;
	}

	zbus_chan_pub(&epilogue_channel, &message, K_SECONDS(1));
}

static void conserve_routine(void)
{
	const struct zbus_channel *channel;

	while (zbus_sub_wait(&conserve_subscriber, &channel, K_FOREVER) == 0) {
		if (channel == &conserve_channel) {
			conserve_handle_conserve_message(channel);
		}

		if (conserve_epilogue_is_imminent()) {
			conserve_request_epilogue();
			break;
		}
	}
}

K_THREAD_DEFINE(conserve_thread, CONFIG_MAIN_STACK_SIZE, conserve_routine, NULL, NULL, NULL,
		3, 0, 0);
