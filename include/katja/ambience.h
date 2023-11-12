#ifndef KATJA_AMBIENCE_H
#define KATJA_AMBIENCE_H

enum ambience {
	AMBIENCE_NIGHT = 0,
	AMBIENCE_TWILIGHT,
	AMBIENCE_DAYLIGHT,
};

int ambience_get(enum ambience *amb);

#endif /* KATJA_AMBIENCE_H */
