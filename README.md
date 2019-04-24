# kuma
kuma is a multi-platform support network library developed in C++. It implements interfaces for TCP/UDP/Multicast/HTTP/HTTP2/WebSocket/timer that drove by event loop. kuma supports epoll/poll/WSAPoll/IOCP/kqueue/select on platform Linux/Windows/OSX/iOS/Android.

## Features

* Support epoll, poll/WSAPoll, IOCP, kqueue and select
* Portable/cross platform(linux, windows, macOS, iOS and Android)
* Support OpenSSL 1.0 and 1.1
* IPv6 support
* Flexible/Scalable timer manager
* Support HTTP compression gzip and deflate
* Support secure HTTP/2 and HTTP/1.1 upgrade to HTTP/2 (rfc7540, rfc7541)
* Support WebSocket(rfc6455) and extension permessage_deflate(rfc7692)
* Support WebSocket over HTTP/2(rfc8441)
* All interface objects, except Timer, are not thread-safe, must run on their EventLoop thread, but close method is thread-safe


## Build
```
define macro KUMA_HAS_OPENSSL to enable openssl
```

### iOS
```
open project bld/ios/kuma with xcode and build it
```

### OSX
```
open project bld/osx/kuma with xcode and build it
```

### Windows
```
open bld/msvc/kuma.sln with VS2017 and build it
```

### Linux
```
$ cd src
$ make
```

### Android
```
$ cd src/jni
$ ndk-build
```

## OpenSSL
```
certificates location is by default in /path-to-your-excutable/cert.

copy all CA certificates used to cert/ca.pem
copy your server certificate to cert/server.pem
copy your server private key to cert/server.key
```

## Simple example
Please refer to [test](https://github.com/Jamol/kuma/tree/master/test) for more examples
### client
```
#include "kmapi.h"
#include "util/defer.h"

using namespace kuma;

int main(int argc, char *argv[])
{
    kuma::init();
    DEFER([]{
        kuma::fini();
    });
    
    EventLoop main_loop(PollType::NONE);
    if (!main_loop.init()) {
        printf("failed to init EventLoop\n");
        return -1;
    }
    
    WebSocket ws(&main_loop, "HTTP/1.1");
    ws.setOpenCallback([] (KMError err) {
        printf("ws.onOpen, err=%d\n", err);
    });
    ws.setDataCallback([] (KMBuffer &data, bool is_text, bool is_fin) {
        printf("ws.onData, len=%lu\n", data.chainLength());
    });
    ws.setWriteCallback([] (KMError err) {
        printf("ws.onWrite, write available\n");
    });
    ws.setErrorCallback([] (KMError err) {
        printf("ws.onError, err=%d\n", err);
    });
    ws.setSubprotocol("jws");
    ws.setOrigin("www.jamol.cn");
    ws.connect("wss://127.0.0.1:8443/");
    
    Timer timer(&main_loop);
    timer.schedule(1000, TimerMode::ONE_SHOT, [] {
        printf("onTimer\n");
    });
    
    main_loop.loop();
    return 0;
}
```
### server
```
#include "kmapi.h"
#include "util/defer.h"

using namespace kuma;

int main(int argc, char *argv[])
{
    kuma::init();
    DEFER([]{
        kuma::fini();
    });
    
    EventLoop main_loop(PollType::NONE);
    if (!main_loop.init()) {
        printf("failed to init EventLoop\n");
        return -1;
    }
    
    WebSocket ws(&main_loop);
    ws.setOpenCallback([] (KMError err) {
        printf("ws.onOpen, err=%d\n", err);
    });
    ws.setDataCallback([] (KMBuffer &data, bool is_text, bool is_fin) {
        printf("ws.onData, len=%lu\n", data.chainLength());
    });
    ws.setWriteCallback([] (KMError err) {
        printf("ws.onWrite, write available\n");
    });
    ws.setErrorCallback([] (KMError err) {
        printf("ws.onError, err=%d\n", err);
    });
    
    TcpListener server(&main_loop);
    server.setAcceptCallback([&ws] (SOCKET_FD fd, const char* ip, uint16_t port) -> bool {
        printf("server.onAccept, ip=%s\n", ip);
        ws.setSslFlags(SSL_ENABLE);
        ws.attachFd(fd, nullptr, [] (KMError err) {
            printf("ws.onHandshake, err=%d\n", err);
            return true;
        });
        return true;
    });
    server.setErrorCallback([] (KMError err) {
        printf("server.onError, err=%d\n", err);
    });
    auto ret = server.startListen("0.0.0.0", 8443);
    if (ret != KMError::NOERR) {
        printf("failed to listen on 8443\n");
    }
    
    main_loop.loop();
    return 0;
}
```


