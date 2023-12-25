#ifndef KATJA_EPILOGUE_H
#define KATJA_EPILOGUE_H

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

enum epilogue_await {
	/* Power off until charger is connected */
	EPILOGUE_AWAIT_CHARGER = BIT(0),
	/* Power off until movement is detected */
	EPILOGUE_AWAIT_MOVEMENT = BIT(1),
	/* Power off for a moment */
	EPILOGUE_AWAIT_MOMENT = BIT(2),
};

struct epilogue_message {
	enum epilogue_await await;
};

ZBUS_CHAN_DECLARE(epilogue_channel);
ZBUS_OBS_DECLARE(epilogue_listener);

#endif /* KATJA_EPILOGUE_H */
