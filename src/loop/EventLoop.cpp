/* Copyright (c) 2014, Fengping Bao <jamol@live.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "EventLoop.h"
#include "internal.h"
#include "util/kmdp.h"
#include "util/kmqueue.h"
#include "util/kmtimer.h"

KUMA_NS_BEGIN

IOPoll* createIOPoll();

EventLoop::EventLoop(uint32_t max_wait_time_ms)
{
    stopLoop_ = false;
    poll_ = createIOPoll();
    max_wait_time_ms_ = max_wait_time_ms;
    timer_mgr_ = new KM_Timer_Manager();
}

EventLoop::~EventLoop()
{
    if(poll_) {
        delete poll_;
        poll_ = NULL;
    }
    delete timer_mgr_;
    timer_mgr_ = NULL;
}

bool EventLoop::init()
{
    if(!poll_->init()) {
        return false;
    }
    return true;
}

int EventLoop::registerIOCallback(int fd, uint32_t events, IOCallback &cb)
{
    EventCallback ev([=, &cb] {
        if(ioCallbacks_.find(fd) != ioCallbacks_.end()) {
            return ;
        }
        auto r = ioCallbacks_.emplace(fd, cb);
        int ret = poll_->register_fd(fd, events, &(r.first->second));
        if(ret != KUMA_ERROR_NOERR) {
            ioCallbacks_.erase(r.first);
            return ;
        }
    });
    return runInEventLoop(std::move(ev));
}

int EventLoop::unregisterIOCallback(int fd, bool close_fd)
{
    EventCallback ev([=] {
        poll_->unregister_fd(fd);
        ioCallbacks_.erase(fd);
        if(close_fd) {
            closesocket(fd);
        }
    });
    return runInEventLoop(std::move(ev));
}

void EventLoop::loop()
{
    while (!stopLoop_) {
        EventCallback cb;
        while (!stopLoop_ && eventQueue_.dequeue(cb)) {
            if(cb) {
                cb();
            }
        }
        unsigned long remain_time_ms = max_wait_time_ms_;
        timer_mgr_->check_expire(&remain_time_ms);
        if(remain_time_ms > max_wait_time_ms_) {
            remain_time_ms = max_wait_time_ms_;
        }
        poll_->wait((uint32_t)remain_time_ms);
    }
}

int EventLoop::runInEventLoop(EventCallback &cb)
{
    eventQueue_.enqueue(cb);
    poll_->notify();
    return KUMA_ERROR_NOERR;
}

int EventLoop::runInEventLoop(EventCallback &&cb)
{
    eventQueue_.enqueue(std::move(cb));
    poll_->notify();
    return KUMA_ERROR_NOERR;
}

#if defined(KUMA_OS_LINUX)
IOPoll* createEPoll();
#elif defined(KUMA_OS_MAC)
IOPoll* createVPoll();
#else
IOPoll* createSelectPoll();
#endif

IOPoll* createIOPoll()
{
#ifdef KUMA_OS_WIN
    return createSelectPoll();
#elif defined(KUMA_OS_LINUX)
    return createEPoll();
#elif defined(KUMA_OS_MAC)
    return createVPoll();
#else
    return createSelectPoll();
#endif
}

KUMA_NS_END
