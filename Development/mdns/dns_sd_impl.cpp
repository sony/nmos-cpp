#include "mdns/dns_sd_impl.h"
#include "detail/pragma_warnings.h"

#ifdef _WIN32
// "Porting Socket Applications to Winsock" is a useful reference
// See https://msdn.microsoft.com/en-us/library/windows/desktop/ms740096(v=vs.85).aspx
#else
// POSIX
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
inline bool dnssd_SocketValid(DNSServiceRefSockFD_t fd) { return fd != INVALID_SOCKET; }
#else
inline bool dnssd_SocketValid(DNSServiceRefSockFD_t fd) { return fd >= 0; }
#endif


struct _DNSServiceCancellationToken_t
{
#ifdef _WIN32
    DNSServiceRefSockFD_t fd;
#else
    DNSServiceRefSockFD_t fds[2];
#endif
};

static DNSServiceErrorType DNSServiceCancellationTokenClose(DNSServiceCancellationToken cancelToken)
{
    if (0 == cancelToken) return kDNSServiceErr_NoError;
    _DNSServiceCancellationToken_t& ct = *cancelToken;
#ifdef _WIN32
    int res = ::closesocket(ct.fd);
    if (0 != res) return kDNSServiceErr_Unknown;
#else
    int res0 = ::close(ct.fds[0]);
    int res1 = ::close(ct.fds[1]);
    if (0 != res0) return kDNSServiceErr_Unknown;
    if (0 != res1) return kDNSServiceErr_Unknown;
#endif
    return kDNSServiceErr_NoError;
}

DNSServiceErrorType DNSServiceCreateCancellationToken(DNSServiceCancellationToken* cancelToken)
{
    // The cancellation token is based on the "self-pipe trick".
    // On Windows, a datagram socket connected to itself is used to similar effect.
    // See https://cr.yp.to/docs/selfpipe.html
    // and https://stackoverflow.com/questions/3333361/how-to-cancel-waiting-in-select-on-windows
    // and https://lwn.net/Articles/177897/ etc.

    if (0 == cancelToken) return kDNSServiceErr_BadParam;
    _DNSServiceCancellationToken_t ct{};
#ifdef _WIN32
    ct.fd = (DNSServiceRefSockFD_t)socket(AF_INET, SOCK_DGRAM, 0);
    if (!dnssd_SocketValid(ct.fd)) return kDNSServiceErr_Unknown;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int res = bind(ct.fd, (sockaddr*)&addr, sizeof(addr));
    if (0 != res) return DNSServiceCancellationTokenClose(&ct), kDNSServiceErr_Unknown;
    int size = sizeof(addr);
    res = getsockname(ct.fd, (sockaddr*)&addr, &size);
    if (0 != res) return DNSServiceCancellationTokenClose(&ct), kDNSServiceErr_Unknown;
    res = connect(ct.fd, (sockaddr*)&addr, sizeof(addr));
    if (0 != res) return DNSServiceCancellationTokenClose(&ct), kDNSServiceErr_Unknown;
#else
    int res = pipe(ct.fds);
    if (0 != res || !dnssd_SocketValid(ct.fds[0]) || !dnssd_SocketValid(ct.fds[1])) return kDNSServiceErr_Unknown;
    res = fcntl(ct.fds[0], F_SETFL, fcntl(ct.fds[0], F_GETFL, 0) | O_NONBLOCK);
    if (0 != res) return DNSServiceCancellationTokenClose(&ct), kDNSServiceErr_Unknown;
    res = fcntl(ct.fds[1], F_SETFL, fcntl(ct.fds[1], F_GETFL, 0) | O_NONBLOCK);
    if (0 != res) return DNSServiceCancellationTokenClose(&ct), kDNSServiceErr_Unknown;
#endif
    *cancelToken = new _DNSServiceCancellationToken_t(ct);
    return kDNSServiceErr_NoError;
}

DNSServiceErrorType DNSServiceCancel(DNSServiceCancellationToken cancelToken)
{
    if (0 == cancelToken) return kDNSServiceErr_BadParam;
    _DNSServiceCancellationToken_t& ct = *cancelToken;
    // Write a byte to make the cancel fd readable; it is never actually read
    // so the cancellation token is single-use
#ifdef _WIN32
    int res = send(ct.fd, "x", 1, 0); // socket
#else
    int res = write(ct.fds[1], "x", 1); // pipe
#endif
    if (res < 0) return kDNSServiceErr_Unknown;
    return kDNSServiceErr_NoError;
}

DNSServiceErrorType DNSServiceCancellationTokenDeallocate(DNSServiceCancellationToken cancelToken)
{
    if (0 == cancelToken) return kDNSServiceErr_NoError;
    _DNSServiceCancellationToken_t ct = *cancelToken;
    delete cancelToken;
    return DNSServiceCancellationTokenClose(&ct);
}

// Wait for a DNSServiceRef to become ready for at most timeout milliseconds (which may be zero)
// On Linux, don't use select, which is the recommended practice, to avoid problems with file descriptors
// greater than or equal to FD_SETSIZE.
// See https://developer.apple.com/documentation/dnssd/1804696-dnsserviceprocessresult
DNSServiceErrorType DNSServiceWaitForReply(DNSServiceRef sdRef, int timeout_millis, DNSServiceCancellationToken cancel)
{
    DNSServiceRefSockFD_t fd = DNSServiceRefSockFD(sdRef);
    if (!dnssd_SocketValid(fd))
    {
        return kDNSServiceErr_BadParam;
    }
#ifdef _WIN32
    DNSServiceRefSockFD_t cancel_fd = 0 != cancel ? cancel->fd : INVALID_SOCKET;
#else
    DNSServiceRefSockFD_t cancel_fd = 0 != cancel ? cancel->fds[0] : -1;
#endif

#ifdef _WIN32

    // FD_SETSIZE doesn't present the same limitation on Windows because it controls the
    // maximum number of file descriptors in the set, not the largest supported value
    // See https://msdn.microsoft.com/en-us/library/windows/desktop/ms740142(v=vs.85).aspx

    fd_set readfds;
    FD_ZERO(&readfds);
    PRAGMA_WARNING_PUSH
    PRAGMA_WARNING_DISABLE_CONDITIONAL_EXPRESSION_IS_CONSTANT
    FD_SET(fd, &readfds);
    if (dnssd_SocketValid(cancel_fd)) FD_SET(cancel_fd, &readfds);
    PRAGMA_WARNING_POP

    // wait for up to timeout milliseconds (converted to secs and usecs)
    struct timeval tv{ timeout_millis / 1000, (timeout_millis % 1000) * 1000 };
    // first parameter is ignored on Windows but required e.g. on Linux
    auto nfds = dnssd_SocketValid(cancel_fd) && cancel_fd > fd ? cancel_fd + 1 : fd + 1;
    int res = select((int)nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
    // map cancelled (or at least fd not readable) to timed out
    if (res > 0 && !FD_ISSET(fd, &readfds)) res = 0;

#else

    // Use poll in order to be able to wait for a file descriptor larger than FD_SETSIZE

    pollfd pfd_read[2];
    pfd_read[0] = { fd, POLLIN, 0 };
    if (dnssd_SocketValid(cancel_fd)) pfd_read[1] = { cancel_fd, POLLIN, 0 };
    int res = poll(&pfd_read[0], dnssd_SocketValid(cancel_fd) ? 2 : 1, timeout_millis);
    // map cancelled (or at least fd not readable) to timed out
    if (res > 0 && (pfd_read[0].revents & POLLIN) == 0) res = 0;

#endif

    // both select and poll return values as follows...
    if (res < 0)
    {
        // WSAGetLastError/errno would give more info
        return kDNSServiceErr_Unknown;
    }
    else if (res == 0)
    {
        // timeout has expired or the operation has been cancelled
        return kDNSServiceErr_Timeout_;
    }
    else
    {
        return kDNSServiceErr_NoError;
    }
}

// Read a reply from the daemon, calling the appropriate application callback
// This call will block for at most timeout milliseconds.
DNSServiceErrorType DNSServiceProcessResult(DNSServiceRef sdRef, int timeout_millis, DNSServiceCancellationToken cancel)
{
    DNSServiceErrorType errorCode = DNSServiceWaitForReply(sdRef, timeout_millis, cancel);
    return errorCode == kDNSServiceErr_NoError ? DNSServiceProcessResult(sdRef) : errorCode;
}
