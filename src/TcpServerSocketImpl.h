#ifndef __TcpServerSocketImpl_H__
#define __TcpServerSocketImpl_H__

#include "kmdefs.h"
#include "evdefs.h"

KUMA_NS_BEGIN

class EventLoopImpl;

class TcpServerSocketImpl
{
public:
    typedef std::function<void(SOCKET_FD)> AcceptCallback;
    typedef std::function<void(int)> ErrorCallback;
    
    TcpServerSocketImpl(EventLoopImpl* loop);
    ~TcpServerSocketImpl();
    
    int startListen(const char* host, uint16_t port);
    int stopListen(const char* host, uint16_t port);
    int close();
    
    void setAcceptCallback(AcceptCallback& cb) { cb_accept_ = cb; }
    void setErrorCallback(ErrorCallback& cb) { cb_error_ = cb; }
    void setAcceptCallback(AcceptCallback&& cb) { cb_accept_ = std::move(cb); }
    void setErrorCallback(ErrorCallback&& cb) { cb_error_ = std::move(cb); }
    
    SOCKET_FD getFd() { return fd_; }
    
protected:
    const char* getObjKey();
    
private:
    void setSocketOption();
    void ioReady(uint32_t events);
    void onAccept();
    void onClose(int err);
    
private:
    void cleanup();
    
private:
    SOCKET_FD       fd_;
    EventLoopImpl*  loop_;
    bool            registered_;
    uint32_t        flags_;
    bool            stopped_;
    
    AcceptCallback  cb_accept_;
    ErrorCallback   cb_error_;
};

KUMA_NS_END

#endif