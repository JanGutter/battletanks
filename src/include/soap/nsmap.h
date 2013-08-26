#ifndef NSMAP_H_
#define NSMAP_H_

#include "soap/soapH.h"
#if 0
SOAP_NMAC struct Namespace soap_namespaces[] =
{
	{"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
	{"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"ns1", "http://challenge.entelect.co.za/", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
#endif
extern SOAP_NMAC struct Namespace namespaces[];
#endif
