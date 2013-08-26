/* soapChallengeServiceSoapBindingProxy.h
   Generated by gSOAP 2.8.15 from generated/ChallengeService.h

Copyright(C) 2000-2013, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under ONE of the following licenses:
GPL or Genivia's license for commercial use.
This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
*/

#ifndef soapChallengeServiceSoapBindingProxy_H
#define soapChallengeServiceSoapBindingProxy_H
#include "soap/soapH.h"

class SOAP_CMAC ChallengeServiceSoapBindingProxy
{ public:
	struct soap *soap;
	bool own;
	/// Endpoint URL of service 'ChallengeServiceSoapBindingProxy' (change as needed)
	const char *soap_endpoint;
	/// Constructor
	ChallengeServiceSoapBindingProxy();
	/// Constructor to use/share an engine state
	ChallengeServiceSoapBindingProxy(struct soap*);
	/// Constructor with endpoint URL
	ChallengeServiceSoapBindingProxy(const char *url);
	/// Constructor with engine input+output mode control
	ChallengeServiceSoapBindingProxy(soap_mode iomode);
	/// Constructor with URL and input+output mode control
	ChallengeServiceSoapBindingProxy(const char *url, soap_mode iomode);
	/// Constructor with engine input and output mode control
	ChallengeServiceSoapBindingProxy(soap_mode imode, soap_mode omode);
	/// Destructor frees deserialized data
	virtual	~ChallengeServiceSoapBindingProxy();
	/// Initializer used by constructors
	virtual	void ChallengeServiceSoapBindingProxy_init(soap_mode imode, soap_mode omode);
	/// Delete all deserialized data (with soap_destroy and soap_end)
	virtual	void destroy();
	/// Delete all deserialized data and reset to default
	virtual	void reset();
	/// Disables and removes SOAP Header from message
	virtual	void soap_noheader();
	/// Get SOAP Header structure (NULL when absent)
	virtual	const SOAP_ENV__Header *soap_header();
	/// Get SOAP Fault structure (NULL when absent)
	virtual	const SOAP_ENV__Fault *soap_fault();
	/// Get SOAP Fault string (NULL when absent)
	virtual	const char *soap_fault_string();
	/// Get SOAP Fault detail as string (NULL when absent)
	virtual	const char *soap_fault_detail();
	/// Close connection (normally automatic, except for send_X ops)
	virtual	int soap_close_socket();
	/// Force close connection (can kill a thread blocked on IO)
	virtual	int soap_force_close_socket();
	/// Print fault
	virtual	void soap_print_fault(FILE*);
#ifndef WITH_LEAN
	/// Print fault to stream
#ifndef WITH_COMPAT
	virtual	void soap_stream_fault(std::ostream&);
#endif

	/// Put fault into buffer
	virtual	char *soap_sprint_fault(char *buf, size_t len);
#endif

	/// Web service operation 'getStatus' (returns error code or SOAP_OK)
	virtual	int getStatus(ns1__getStatus *ns1__getStatus_, ns1__getStatusResponse *ns1__getStatusResponse_) { return this->getStatus(NULL, NULL, ns1__getStatus_, ns1__getStatusResponse_); }
	virtual	int getStatus(const char *endpoint, const char *soap_action, ns1__getStatus *ns1__getStatus_, ns1__getStatusResponse *ns1__getStatusResponse_);

	/// Web service operation 'setAction' (returns error code or SOAP_OK)
	virtual	int setAction(ns1__setAction *ns1__setAction_, ns1__setActionResponse *ns1__setActionResponse_) { return this->setAction(NULL, NULL, ns1__setAction_, ns1__setActionResponse_); }
	virtual	int setAction(const char *endpoint, const char *soap_action, ns1__setAction *ns1__setAction_, ns1__setActionResponse *ns1__setActionResponse_);

	/// Web service operation 'login' (returns error code or SOAP_OK)
	virtual	int login(ns1__login *ns1__login_, ns1__loginResponse *ns1__loginResponse_) { return this->login(NULL, NULL, ns1__login_, ns1__loginResponse_); }
	virtual	int login(const char *endpoint, const char *soap_action, ns1__login *ns1__login_, ns1__loginResponse *ns1__loginResponse_);

	/// Web service operation 'setActions' (returns error code or SOAP_OK)
	virtual	int setActions(ns1__setActions *ns1__setActions_, ns1__setActionsResponse *ns1__setActionsResponse_) { return this->setActions(NULL, NULL, ns1__setActions_, ns1__setActionsResponse_); }
	virtual	int setActions(const char *endpoint, const char *soap_action, ns1__setActions *ns1__setActions_, ns1__setActionsResponse *ns1__setActionsResponse_);
};
#endif
