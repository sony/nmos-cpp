#ifndef MDNS_DNS_SD_IMPL_H
#define MDNS_DNS_SD_IMPL_H

#include "dns_sd.h"

// dnssd_sock_t was added relatively recently (765.1.2)
typedef decltype(DNSServiceRefSockFD(0)) DNSServiceRefSockFD_t;

// kDNSServiceErr_Timeout was added a long time ago (320.5) but did not exist in Tiger
// and is not defined by the Avahi compatibility layer
const DNSServiceErrorType kDNSServiceErr_Timeout_ = -65568;

// DNSServiceGetAddrInfo was added a long time ago (161.1) but did not exist in Tiger
// and is not defined by the Avahi compatibility layer, so getaddrinfo must be relied on instead
#if _DNS_SD_H+0 >= 1610100
#define HAVE_DNSSERVICEGETADDRINFO
#endif

#ifdef _WIN32
// kIPv6IfIndexBase was added a really long time ago (107.1) in the Windows implementation
// because "Windows does not have a uniform scheme for IPv4 and IPv6 interface indexes"
const uint32_t kIPv6IfIndexBase = 10000000L;
#endif

typedef struct _DNSServiceCancellationToken_t *DNSServiceCancellationToken;

DNSServiceErrorType DNSServiceCreateCancellationToken(DNSServiceCancellationToken* cancelToken);
DNSServiceErrorType DNSServiceCancel(DNSServiceCancellationToken cancelToken);
DNSServiceErrorType DNSServiceCancellationTokenDeallocate(DNSServiceCancellationToken cancelToken);

// Wait for a DNSServiceRef to become ready for at most timeout milliseconds (which may be zero)
// On Linux, don't use select, which is the recommended practice, to avoid problems with file descriptors
// greater than or equal to FD_SETSIZE.
// See https://developer.apple.com/documentation/dnssd/1804696-dnsserviceprocessresult
DNSServiceErrorType DNSServiceWaitForReply(DNSServiceRef sdRef, int timeout_millis, DNSServiceCancellationToken cancelToken = 0);

// Read a reply from the daemon, calling the appropriate application callback
// This call will block for at most timeout milliseconds.
DNSServiceErrorType DNSServiceProcessResult(DNSServiceRef sdRef, int timeout_millis, DNSServiceCancellationToken cancelToken = 0);

#endif
