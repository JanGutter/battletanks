/*
 * NetworkCore.h
 *
 *  Created on: 09 Aug 2013
 *      Author: Jan Gutter
 */

#ifndef NETWORKCORE_H_
#define NETWORKCORE_H_

#include "soap/soapChallengeServiceSoapBindingProxy.h"
#include "soap/nsmap.h"
#include "consts.h"
#include "PlayoutState.h"
#include <string>

using namespace std;

#define POLICY_RANDOM 0
#define POLICY_GREEDY 1
#define POLICY_FIXED 2

class NetworkCore {
private:
	PlayoutState state;
	UtilityScores u;
	ChallengeServiceSoapBindingProxy s;
	string myname;
	bool state_synced;
	int soaperr;
public:
	int policy;
	NetworkCore(const char* soap_endpoint);
	void login();
	void play();
};

extern void network_play(const char* soap_endpoint);

#endif /* NETWORKCORE_H_ */
