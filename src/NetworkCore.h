//============================================================================
// Name        : NetworkCore.h
// Author      : Jan Gutter
// Copyright   : To the extent possible under law, Jan Gutter has waived all
//             : copyright and related or neighboring rights to this work.
//             : For more information, go to:
//             : http://creativecommons.org/publicdomain/zero/1.0/
//             : or consult the README and COPYING files
// Description : Network Core interfaces
//============================================================================

#ifndef NETWORKCORE_H_
#define NETWORKCORE_H_

#include "soap/soapChallengeServiceSoapBindingProxy.h"
#include "soap/nsmap.h"
#include "consts.h"
#include "PlayoutState.h"
#include <string>
#include <utility>
#include <algorithm>
#include <functional>

using namespace std;

#define POLICY_RANDOM 0
#define POLICY_GREEDY 1
#define POLICY_FIXED 2
#define POLICY_MCTS 3

class NetworkCore {
private:
	PlayoutState* state;
	ChallengeServiceSoapBindingProxy s;
	string myname;
	bool state_synced;
	int soaperr;
public:
	int policy;
	NetworkCore(const char* soap_endpoint);
	void login();
	void play();
	~NetworkCore();
};

extern void network_play(const char* soap_endpoint);

#endif /* NETWORKCORE_H_ */
