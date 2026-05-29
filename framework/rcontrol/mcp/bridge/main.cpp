#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
using sock_t = SOCKET;
static const sock_t INVALID_SOCK = INVALID_SOCKET;
static int closeSock(sock_t s) { return ::closesocket(s); }
#else
  #include <netdb.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #include <csignal>
using sock_t = int;
static const sock_t INVALID_SOCK = -1;
static int closeSock(sock_t s) { return ::close(s); }
#endif

namespace {
sock_t connectTo(const char* host, const char* port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (::getaddrinfo(host, port, &hints, &res) != 0 || !res) {
        return INVALID_SOCK;
    }
    sock_t fd = INVALID_SOCK;
    for (addrinfo* p = res; p; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == INVALID_SOCK) {
            continue;
        }
        if (::connect(fd, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
            break;
        }
        closeSock(fd);
        fd = INVALID_SOCK;
    }
    ::freeaddrinfo(res);
    return fd;
}

class LineReader
{
public:
    explicit LineReader(sock_t fd)
        : m_fd(fd) {}
    bool readLine(std::string& out)
    {
        for (;;) {
            const size_t nl = m_buf.find('\n');
            if (nl != std::string::npos) {
                out = m_buf.substr(0, nl);
                if (!out.empty() && out.back() == '\r') {
                    out.pop_back();
                }
                m_buf.erase(0, nl + 1);
                return true;
            }
            char tmp[4096];
            const int n = ::recv(m_fd, tmp, sizeof(tmp), 0);
            if (n <= 0) {
                return false;
            }
            m_buf.append(tmp, static_cast<size_t>(n));
        }
    }

private:
    sock_t m_fd;
    std::string m_buf;
};

bool sendAll(sock_t fd, const std::string& data)
{
    size_t sent = 0;
    while (sent < data.size()) {
        const int m = ::send(fd, data.data() + sent, static_cast<int>(data.size() - sent), 0);
        if (m <= 0) {
            return false;
        }
        sent += static_cast<size_t>(m);
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
    const char* port = (argc > 2) ? argv[2] : "2212";

#if defined(_WIN32)
    WSADATA wsa;
    if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::fprintf(stderr, "WSAStartup failed\n");
        return EXIT_FAILURE;
    }
#endif

#if !defined(_WIN32)
    std::signal(SIGPIPE, SIG_IGN);
#endif

    const sock_t fd = connectTo(host, port);
    if (fd == INVALID_SOCK) {
        std::fprintf(stderr, "cannot connect to %s:%s\n", host, port);
        #if defined(_WIN32)
        ::WSACleanup();
        #endif
        return EXIT_FAILURE;
    }

    LineReader reader(fd);
    std::string request;

    while (std::getline(std::cin, request)) {
        if (!request.empty() && request.back() == '\r') {
            request.pop_back();
        }

        if (request.empty()) {
            continue;
        }

        if (!sendAll(fd, request + "\n")) {
            std::fprintf(stderr, "send failed\n");
            break;
        }

        std::string response;
        if (!reader.readLine(response)) {
            std::fprintf(stderr, "connection closed by server\n");
            break;
        }

        std::fputs(response.c_str(), stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }

    closeSock(fd);
#if defined(_WIN32)
    ::WSACleanup();
#endif
    return EXIT_SUCCESS;
}
