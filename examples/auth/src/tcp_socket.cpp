#include "tcp_socket.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <util/string_type.h>

using namespace std;

void
TcpSocket::init_socket_lib()
{
#ifdef YBUTIL_WINDOWS
    static bool did_it = false;
    static WSAData wsaData;
    if (!did_it) {
        WORD versionRequested = MAKEWORD(1, 1);
        int err = WSAStartup(versionRequested, &wsaData);
        if (err != 0) {
            throw SocketEx("init_socket_lib",
                    "WSAStartup failed with error: " +
                    Yb::to_stdstring(err));
        }
        did_it = true;
    }
#endif
}

SOCKET
TcpSocket::create()
{
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == s)
        throw SocketEx("create", get_last_error());
    return s;
}

string
TcpSocket::get_last_error()
{
    char buf[1024];
    size_t buf_sz = sizeof(buf);
#ifdef YBUTIL_WINDOWS
    int err;
    LPTSTR msg_buf;
    err = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msg_buf,
        0,
        NULL);
    CharToOemBuff(msg_buf, buf, buf_sz);
    LocalFree(msg_buf);
#else
#if ! _GNU_SOURCE
    strerror_r(errno, buf, buf_sz);
#else
    strncpy(buf, strerror_r(errno, buf, buf_sz), buf_sz);
#endif
#endif
    buf[buf_sz - 1] = 0;
    return string(buf);
}

void
TcpSocket::bind(int port)
{
    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    SockOpt yes = 1;
    setsockopt(s_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (::bind(s_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        throw SocketEx("bind", get_last_error());
}

void
TcpSocket::listen()
{
    ::listen(s_, 3);
}

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif // _MSC_VER

SOCKET
TcpSocket::accept(string *ip_addr, int *ip_port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    struct sockaddr *p_addr = NULL;
#ifdef YBUTIL_WINDOWS
    typedef int socklen_t;
#endif
    socklen_t addr_len = sizeof(addr), *p_addr_len = NULL;
    if (ip_addr || ip_port) {
        p_addr = (struct sockaddr *)&addr;
        p_addr_len = &addr_len;
    }
    SOCKET s2 = ::accept(s_, p_addr, p_addr_len);
    if (INVALID_SOCKET == s2)
        throw SocketEx("accept", get_last_error());
    if (ip_port)
        *ip_port = ntohs(*(unsigned short *)&addr.sin_port);
    if (ip_addr) {
        unsigned ip = ntohl(*(unsigned *)&addr.sin_addr);
        char buf[40];
        sprintf(buf, "%d.%d.%d.%d",
                ip >> 24, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
        *ip_addr = string(buf);
    }
    return s2;
}

bool
TcpSocket::read_chunk()
{
    if (buf_pos_ < buf_.size())
        return true;
    buf_pos_ = 0;
    buf_.clear();
    fd_set rfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&efds);
    FD_SET(s_, &rfds);
    FD_SET(s_, &efds);
    struct timeval t;
    t.tv_sec = timeout_ / 1000;
    t.tv_usec = (timeout_ % 1000) * 1000;
    int res = ::select(s_ + 1, &rfds, NULL, &efds, &t);
    if (!res)
        throw SocketEx("select", "timeout");
    if (res != 1 && res != 2)
        throw SocketEx("select", get_last_error());
    if (FD_ISSET(s_, &efds))
        throw SocketEx("select", "socket exception");
    int max_chunk = 2500;
    buf_.resize(max_chunk);
    res = ::recv(s_, &buf_[0], max_chunk, 0);
    if (res < 0 || res > max_chunk)
        throw SocketEx("read", get_last_error());
    buf_.resize(res);
    return buf_pos_ < buf_.size();
}

const string
TcpSocket::readline()
{
    string req;
    req.reserve(40);
    while (1) {
        while (buf_pos_ < buf_.size()) {
            char c = buf_[buf_pos_++];
            req.push_back(c);
            if (c == '\n')
                return req;
        }
        if (!read_chunk())
            return req;
    }
}

const string
TcpSocket::read(size_t n)
{
    string r;
    r.reserve(n);
    size_t pos = 0;
    while (pos < n) {
        if (buf_pos_ < buf_.size()) {
            size_t min_sz = buf_.size() - buf_pos_;
            if (n - pos < min_sz)
                min_sz = n - pos;
            r.append(buf_, buf_pos_, min_sz);
            buf_pos_ += min_sz;
            pos += min_sz;
        }
        if (pos < n) {
            if (!read_chunk())
                throw SocketEx("read", "short read");
        }
    }
    return r;
}

void
TcpSocket::write(const string &msg)
{
    int count = ::send(s_, msg.c_str(), msg.size(), 0);
    if (-1 == count)
        throw SocketEx("write", get_last_error());
    if (static_cast<size_t>(count) < msg.size())
        throw SocketEx("write", "short write");
}

void
TcpSocket::close(bool shut_down)
{
    if (shut_down)
        ::shutdown(s_, 2);
#ifdef YBUTIL_WINDOWS
    ::closesocket(s_);
#else
    ::close(s_);
#endif
    s_ = INVALID_SOCKET;
}

// vim:ts=4:sts=4:sw=4:et:
