//============================================================================
// Name        : sleep.h
// Author      : Jan Gutter
// Copyright   : To the extent possible under law, Jan Gutter has waived all
//             : copyright and related or neighboring rights to this work.
//             : For more information, go to:
//             : http://creativecommons.org/publicdomain/zero/1.0/
//             : or consult the README and COPYING files
// Description : Wrapper to implement Sleep for cross-platforminess
//============================================================================

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
