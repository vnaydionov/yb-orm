#ifndef _AUTH__TCP_SOCKET_H_
#define _AUTH__TCP_SOCKET_H_

#include <util/util_config.h>
#ifdef YBUTIL_WINDOWS
#include <windows.h>
#include <winsock.h>
typedef char SockOpt;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
typedef int SOCKET;
typedef int SockOpt;
#define INVALID_SOCKET -1
#endif
#include <string>
#include <stdexcept>

class SocketEx: public std::runtime_error {
public: SocketEx(const std::string &ctx, const std::string &msg)
    : std::runtime_error(ctx + ": " + msg) {}
};

class TcpSocket {
    SOCKET s_;
public:
    static void init_socket_lib();
    static SOCKET create();
    static std::string get_last_error();
    TcpSocket(SOCKET s = INVALID_SOCKET): s_(s) {};
    bool ok() const { return INVALID_SOCKET == s_; }
    void bind(int port);
    void listen();
    SOCKET accept(std::string *ip_addr = NULL, int *ip_port = NULL);
    std::string readline();
    const std::string read(size_t n);
    void write(const std::string &msg);
    void close(bool shut_down = false);
};

#endif
// vim:ts=4:sts=4:sw=4:et:
