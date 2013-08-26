/*
 * sleep.h
 *
 *  Created on: 11 Aug 2013
 *      Author: Jan Gutter
 */

#ifndef SLEEP_H_
#define SLEEP_H_
#ifdef WIN32
#include <windows.h>

#else

#include <time.h>

inline void Sleep(unsigned int msecs) {
	nanosleep((struct timespec[]){{(msecs/1000), (msecs%1000)*1000000l}}, NULL);
}
#endif

#endif /* SLEEP_H_ */
