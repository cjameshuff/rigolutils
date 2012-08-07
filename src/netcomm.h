
#ifndef NETCOMM_H
#define NETCOMM_H

#include <iostream>
#include <string>
#include <vector>
#include <queue>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "simple_except.h"

class Host;
class Client;
struct Socket {
    struct sockaddr_storage other_addr;
    int sockfd;
    std::string other;
    
    Socket(): sockfd(-1) {}
    ~Socket() {if(sockfd != -1) Close();}
    friend class Host;
    friend class Client;
    
    int Close() {return close(sockfd);}
    int Shutdown(int how) {return shutdown(sockfd, how);}
    int ShutdownRx(int how) {return shutdown(sockfd, 0);}
    int ShutdownTx(int how) {return shutdown(sockfd, 1);}
    int ShutdownRxTx(int how) {return shutdown(sockfd, 2);}
    
    // flags: MSG_OOB, MSG_PEEK, MSG_WAITALL
    int Recv(std::vector<uint8_t> & buf, size_t maxdata, int flags = 0) {
        buf.resize(maxdata);
        ssize_t status = recv(sockfd, &buf[0], maxdata, 0);
        if(status >= 0)
            buf.resize(status);
        return status;
    }
    // flags: MSG_OOB, MSG_DONTROUTE
    int Send(const std::vector<uint8_t> & buf, int flags = 0) {
        return send(sockfd, &buf[0], buf.size(), 0);
    }
};

class FramedConnection {
    int sockfd;
    std::queue<std::vector<uint8_t> *> buffers;
    std::vector<uint8_t> * currentBuffer;
    bool escaped;
    bool good;
    
  public:
    FramedConnection(int s): sockfd(s), currentBuffer(NULL), good(true) {SetNonblocking();}
    
    void SetNonblocking()
    {
        int flags = fcntl(sockfd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, flags);
    }
    
    void SetBlocking()
    {
        int flags = fcntl(sockfd, F_GETFL, 0);
        flags &= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, flags);
    }
    
    bool Good() const {return good;}
    
    bool Poll()
    {
        if(!currentBuffer) {
            escaped = false;
            currentBuffer = new std::vector<uint8_t>;
        }
        
        uint8_t tmpbuf[1024];
        ssize_t status;
        do {
            // Read block of data into tmpbuf, copy to accumulation buffer,
            // breaking into frames.
            // Framing is done SLIP-style:
            // 0xFF is an escape character
            // 0xFF 0xFE escapes the 0xFF character
            // 0xFF 0xFD terminates the current frame
            status = recv(sockfd, tmpbuf, 1024, 0);
            
            // Ignore these errors
            if(status == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
                continue;
            
            if(status < 0)
            {
                good = false;
                throw FormattedError("recv() failed (error %d): %s", errno, strerror(errno));
            }
            
            for(ssize_t j = 0; j < status; ++j)
            {
                if(escaped)
                {
                    if(tmpbuf[j] == 0xFE) {// escaped 0xFF
                        currentBuffer->push_back(0xFF);
                    }
                    else if(tmpbuf[j] == 0xFD) {// break frame
                        buffers.push(currentBuffer);
                        printf("received: \n");
                        for(ssize_t j = 0; j < currentBuffer->size(); ++j)
                            printf(" %02X", (*currentBuffer)[j]);
                        printf("\n");
                        currentBuffer = new std::vector<uint8_t>;
                    }
                    escaped = false;
                }
                else
                {
                    if(tmpbuf[j] == 0xFF)
                        escaped = true;
                    else
                        currentBuffer->push_back(tmpbuf[j]);
                }
            }
        } while(status > 0);
        
        return !buffers.empty();
    }
    
    std::vector<uint8_t> * PopMessage()
    {
        if(buffers.empty())
            return NULL;
        std::vector<uint8_t> * bfr = buffers.front();
        buffers.pop();
        return bfr;
    }
    
    std::vector<uint8_t> * WaitPopMessage() {
        while(buffers.empty())
            Poll();
        return PopMessage();
    }
    
    void SendMessage(uint8_t bfr[], size_t len)
    {
        std::vector<uint8_t> outbfr(len*2);
        printf("sending: \n");
        for(ssize_t j = 0; j < len; ++j)
            printf(" %02X", bfr[j]);
        printf("\n");
        int k = 0;
        for(int j = 0; j < len; ++j)
        {
            if(bfr[j] == 0xFF)
            {
                outbfr[k++] = 0xFF;
                outbfr[k++] = 0xFE;
            }
            else
            {
                outbfr[k++] = bfr[j];
            }
        }
        outbfr[k++] = 0xFF;
        outbfr[k++] = 0xFD;
        ssize_t status = send(sockfd, &outbfr[0], k, 0);
    }
};

class Host {
    int listenfd;
  public:
    Host(const std::string & port, int backlog = 10):
        listenfd(-1)
    {
        int status;
        struct addrinfo hints, * servinfos;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        status = getaddrinfo(NULL, port.c_str(), &hints, &servinfos);
        if(status != 0) throw FormattedError("getaddrinfo() failed with status: %d", status);
        
        listenfd = -1;
        struct addrinfo * servinfo = servinfos;
        while(servinfo != NULL && listenfd == -1)
        {
            listenfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
            if(listenfd == -1) {
                perror("listen socket error");
                servinfo = servinfo->ai_next;
                continue;
            }
            
            int reuse = 1;
            status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
            if(status == -1)
                throw FormattedError("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
            
            fcntl(listenfd, F_SETFL, O_NONBLOCK);
            
            status = bind(listenfd, servinfo->ai_addr, servinfo->ai_addrlen);
            if(status == -1) {
                close(listenfd);
                listenfd = -1;
                perror("bind() failed");
                servinfo = servinfo->ai_next;
                continue;
            }
        }
        
        freeaddrinfo(servinfos);
        
        if(listenfd == -1)
            throw FormattedError("failed to bind");
        
        if(listen(listenfd, backlog) == -1) {
            perror("listen");
            throw FormattedError("listen() failed");
        }
    }
    ~Host() {if(listenfd != -1) close(listenfd);}
    
    void Close() {if(listenfd != -1) close(listenfd); listenfd = -1;}
    
    Socket * Accept()
    {
        struct sockaddr_storage other_addr;
        int sockfd;
        socklen_t sin_size = sizeof(struct sockaddr_storage);
        sockfd = accept(listenfd, (struct sockaddr *)&other_addr, &sin_size);
        if(sockfd == -1)
        {
            if(errno == EWOULDBLOCK)
                return NULL;
            else
                throw FormattedError("accept() failed: %s", strerror(errno));
        }
        
        char s[INET6_ADDRSTRLEN];
        struct sockaddr * sa = (struct sockaddr *)&other_addr;
        if(sa->sa_family == AF_INET)
            inet_ntop(other_addr.ss_family, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
        else
            inet_ntop(other_addr.ss_family, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
        
        Socket * sock = new Socket;
        sock->other_addr = other_addr;
        sock->sockfd = sockfd;
        sock->other = s;
        return sock;
    }
};


Socket * ClientConnect(const std::string & host, const std::string & port)
{
    int status;
    struct addrinfo hints, * servinfos;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfos);
    if(status != 0) throw FormattedError("getaddrinfo() failed with status: %d", status);
    
    int sockfd = -1;
    struct addrinfo * servinfo = servinfos;
    while(servinfo != NULL && sockfd == -1)
    {
        sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if(sockfd == -1) {
            perror("socket error");
            servinfo = servinfo->ai_next;
            continue;
        }
        
        status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
        if(status == -1) {
            close(sockfd);
            sockfd = -1;
            perror("connect error");
            servinfo = servinfo->ai_next;
            continue;
        }
    }
    
    if(sockfd == -1)
        throw FormattedError("failed to connect");
    
    char s[INET6_ADDRSTRLEN];
    struct sockaddr * sa = (struct sockaddr *)&(servinfo->ai_addr);
    if(sa->sa_family == AF_INET)
        inet_ntop(servinfo->ai_family, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
    else
        inet_ntop(servinfo->ai_family, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
    
    Socket * sock = new Socket;
    // sock->other_addr = *sa;
    sock->sockfd = sockfd;
    sock->other = s;
    
    freeaddrinfo(servinfos);
    return sock;
}


#endif // NETCOMM_H
