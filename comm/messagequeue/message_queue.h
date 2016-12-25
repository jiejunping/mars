/*  Copyright (c) 2013-2015 Tencent. All rights reserved.  */
/*
 * MessageQueue.h
 *
 *  Created on: 2013-4-3
 *      Author: yerungui
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "boost/function.hpp"
#include "boost/any.hpp"
#include "boost/smart_ptr.hpp"

#if __cplusplus >= 201103L
#include "boost/static_assert.hpp"
#include "boost/utility/result_of.hpp"
#include "boost/type_traits/is_same.hpp"
#endif

#include "mars/comm/thread/thread.h"

namespace MessageQueue {

typedef uint64_t MessageQueue_t;
typedef boost::function<void ()> AsyncInvokeFunction;

const MessageQueue_t KInvalidQueueID = 0;

struct MessageHandler_t {
    MessageHandler_t(): queue(KInvalidQueueID), seq(0) {}
    bool operator == (const MessageHandler_t& _rhs) const {return queue == _rhs.queue && seq == _rhs.seq;}
    bool operator!=(const MessageHandler_t& _rhs) const {return !operator==(_rhs);}
    bool isbroadcast() const {return 0 == seq;}
    MessageQueue_t queue;
    unsigned int seq;
};

struct MessagePost_t {
    MessagePost_t(): seq(0) {}
    bool operator == (const MessagePost_t& _rhs) const {return reg == _rhs.reg && seq == _rhs.seq;}
    bool operator!=(const MessagePost_t& _rhs) const {return !operator==(_rhs);}
    MessageHandler_t reg;
    unsigned int seq;
};

struct MessageTitle_t {
    MessageTitle_t(): title(0) {}
    template<typename T> MessageTitle_t(const T& _title): title((uintptr_t)_title) { BOOST_STATIC_ASSERT(sizeof(T) <= sizeof(uintptr_t));}

    bool operator == (const MessageTitle_t& _rhs) const { return title == _rhs.title;}
    bool operator!=(const MessageTitle_t& _rhs) const { return !operator==(_rhs);}
    uintptr_t title;
};


struct Message {
    Message(): title(0) {}
    Message(const MessageTitle_t& _title, const boost::any& _body1, const boost::any& _body2)
        : title(_title), body1(_body1), body2(_body2) {}

    template <class F>
    Message(const MessageTitle_t& _title, const F& _func)
    : title(_title), body1(boost::make_shared<AsyncInvokeFunction>()), body2() {
        *boost::any_cast<boost::shared_ptr<AsyncInvokeFunction> >(body1) = _func;
    }

    bool operator == (const Message& _rhs) const {return title == _rhs.title;}

    MessageTitle_t title;
    boost::any body1;
    boost::any body2;
};

enum TMessageTiming {
    kAfter,
    kPeriod,
    kImmediately,
};
    
struct MessageTiming {
    
    MessageTiming(TMessageTiming _timing, int64_t _after, int64_t _period)
        : type(_timing)
        , after(_after)
        , period(_period)
    {}

    MessageTiming(int64_t _after, int64_t _period)
    : type(TMessageTiming::kPeriod)
        , after(_after)
        , period(_period)
    {}

    MessageTiming(int64_t _after)
    : type(TMessageTiming::kAfter)
        , after(_after)
        , period(0)
    {}

    MessageTiming()
    : type(TMessageTiming::kImmediately)
        , after(0)
        , period(0)
    {}

    TMessageTiming type;
    int64_t after;
    int64_t period;
};

typedef boost::function<void (const MessagePost_t& _id, Message& _message)> MessageHandler;

const MessageTiming     KDefTiming;
const MessageHandler_t  KNullHandler;
const MessagePost_t     KNullPost;
const Message           KNullMessage;

inline const MessageHandler_t& Post2Handler(const MessagePost_t& _postid) { return _postid.reg;}
inline const MessageQueue_t& Handler2Queue(const MessageHandler_t& _handler) { return _handler.queue;}
    
MessageQueue_t CurrentThreadMessageQueue();
MessageQueue_t TID2MessageQueue(thread_tid _tid);
thread_tid     MessageQueue2TID(MessageQueue_t _id);

const Message& RuningMessage();
    
MessagePost_t    RuningMessageID();
MessagePost_t    RuningMessageID(const MessageQueue_t& _id);
MessageQueue_t   GetDefMessageQueue();
MessageQueue_t   GetDefTaskQueue();
MessageHandler_t DefAsyncInvokeHandler(const MessageQueue_t& _messagequeue = CurrentThreadMessageQueue());

void WaitForRuningLockEnd(const MessagePost_t&  _message);
void WaitForRuningLockEnd(const MessageHandler_t&  _handler);
void WaitForRuningLockEnd(const MessageQueue_t&  _messagequeueid);
void BreakMessageQueueRunloop(const MessageQueue_t&  _messagequeueid);

MessageHandler_t InstallMessageHandler(const MessageHandler& _handler, bool _recvbroadcast = false, const MessageQueue_t& _messagequeueid = GetDefMessageQueue());
void UnInstallMessageHandler(const MessageHandler_t& _handlerid);

MessagePost_t PostMessage(const MessageHandler_t& _handlerid, const Message& _message, const MessageTiming& _timing = KDefTiming);
MessagePost_t SingletonMessage(bool _replace, const MessageHandler_t& _handlerid, const Message& _message, const MessageTiming& _timing = KDefTiming);
MessagePost_t BroadcastMessage(const MessageQueue_t& _messagequeueid,  const Message& _message, const MessageTiming& _timing = KDefTiming);
MessagePost_t FasterMessage(const MessageHandler_t& _handlerid, const Message& _message, const MessageTiming& _timing = KDefTiming);

bool WaitMessage(const MessagePost_t& _message);
bool FoundMessage(const MessagePost_t& _message);

bool CancelMessage(const MessagePost_t& _postid);
void CancelMessage(const MessageHandler_t& _handlerid);
void CancelMessage(const MessageHandler_t& _handlerid, const MessageTitle_t& _title);

//AsyncInvoke
MessageHandler_t InstallAsyncHandler(const MessageQueue_t& id);

template<class F>
MessagePost_t AsyncInvoke(const F& _func, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(0, _func));
}

template<class F>
MessagePost_t  AsyncInvokeAfter(int64_t _after, const F& _func, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(0, _func), MessageTiming(kAfter, _after, 0));
}

template<class F>
MessagePost_t  AsyncInvokePeriod(int64_t _after, int64_t _period, const F& _func, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(0, _func), MessageTiming(kPeriod, _after, _period));
}

template<class F>
MessagePost_t  AsyncInvoke(const F& _func, const MessageTitle_t& _title, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(_title, _func));
}

template<class F>
MessagePost_t  AsyncInvokeAfter(int64_t _after, const F& _func, const MessageTitle_t& _title, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(_title, _func), MessageTiming(kAfter, _after, 0));
}

template<class F>
MessagePost_t  AsyncInvokePeriod(int64_t _after, int64_t _period, const F& _func, const MessageTitle_t& _title, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(_title, _func), MessageTiming(kPeriod, _after, _period));
}
    
class RunLoop {
 public:
    template<typename F>
    RunLoop(const F& _breaker_func):breaker_func_(_breaker_func) {}
    RunLoop() {}
    
 public:
    void Run();
    
 private:
     RunLoop(const RunLoop&);
     RunLoop& operator=(const RunLoop&);
    
 private:
    boost::function<bool ()> breaker_func_;
    
};
    
class RunloopCond {
public:
    RunloopCond() {};
    virtual ~RunloopCond() {};
    
    static boost::shared_ptr<RunloopCond> CurrentCond();
    
public:
    virtual const boost::typeindex::type_info& type() const = 0;
    virtual void  Wait(ScopedLock& _lock, long _millisecond) = 0;
    virtual void  Notify(ScopedLock& _lock) = 0;
    
private:
    RunloopCond(const RunloopCond&);
    void operator=(const RunloopCond&);
};
    
    
class MessageQueueCreater {
  public:
    MessageQueueCreater(bool _iscreate = false, const char* _msg_queue_name = NULL);
    MessageQueueCreater(boost::shared_ptr<RunloopCond> _breaker, bool _iscreate = false, const char* _msg_queue_name = NULL);
    ~MessageQueueCreater();

    MessageQueue_t GetMessageQueue();
    MessageQueue_t CreateMessageQueue();
    void CancelAndWait();

    static MessageQueue_t CreateNewMessageQueue(const char* _messagequeue_name = NULL);
    static MessageQueue_t CreateNewMessageQueue(boost::shared_ptr<RunloopCond> _breaker, const char* _messagequeue_name = NULL);
    static void ReleaseNewMessageQueue(MessageQueue_t _messagequeue_id); // block api

  private:
    MessageQueueCreater(const MessageQueueCreater&);
    MessageQueueCreater& operator=(const MessageQueueCreater&);

    void __ThreadRunloop();
    static void __ThreadNewRunloop(SpinLock* _sp);

  private:
    Thread                              thread_;
    Mutex                               messagequeue_mutex_;
    MessageQueue_t                      messagequeue_id_;
    boost::shared_ptr<RunloopCond>   breaker_;
};

template <typename R>
class AsyncResult {
  private:
    struct AsyncResultWrapper {
        AsyncResultWrapper(): result_holder(new R), result_valid(false), result(*result_holder) {}
        AsyncResultWrapper(R& _result): result_holder(NULL), result_valid(false), result(_result) {}
        ~AsyncResultWrapper() {
            if (!result_valid && callback_function)
                callback_function(result, false);
        }

        R*  result_holder;

        boost::function<R()> invoke_function;
        boost::function<void (R&, bool)> callback_function;

        bool result_valid;
        R& result;
    };

  public:
    template<typename T>
    AsyncResult(const T& _func)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, R>::value);
#endif
        wrapper_->invoke_function = _func;
    }

    template<typename T>
    AsyncResult(const T& _func, R& _result_holder)
        : wrapper_(new AsyncResultWrapper(_result_holder)) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, R>::value);
#endif
        wrapper_->invoke_function = _func;
    }

    template<typename T, typename C>
    AsyncResult(const T& _func, const C& _callback)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, R>::value);
#endif
        wrapper_->invoke_function = _func;
        wrapper_->callback_function = _callback;
    }

    template<typename T, typename C>
    AsyncResult(const T& _func, R& _result_holder, const C& _callback)
        : wrapper_(new AsyncResultWrapper(_result_holder)) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, R>::value);
#endif
        wrapper_->invoke_function = _func;
        wrapper_->callback_function = _callback;
    }

    void operator()() {
        wrapper_->result = wrapper_->invoke_function();
        wrapper_->result_valid = true;

        if (wrapper_->callback_function)
            wrapper_->callback_function(Result(), true);
    }

    boost::function<R()>& Function() { return wrapper_->invoke_function;}
    boost::function<void (R&, bool)>& CallFunction() { return wrapper_->callback_function;}
    R& Result() { return wrapper_->result;}
    operator bool() const { return wrapper_->result_valid;}

  private:
    AsyncResult& operator=(const AsyncResult&);
    // AsyncResult(const AsyncResult& _ref);

  private:
    boost::shared_ptr<AsyncResultWrapper> wrapper_;
};

template <>
class AsyncResult<void> {
  private:
    struct AsyncResultWrapper {
        AsyncResultWrapper(): result_valid(false) {}
        ~AsyncResultWrapper() {
            if (!result_valid && callback_function)
                callback_function(false);
        }

        boost::function<void ()> invoke_function;
        boost::function<void (bool)> callback_function;
        bool result_valid;
    };

  public:
    template<typename T>
    AsyncResult(const T& _func)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, void>::value);
#endif
        wrapper_->invoke_function = _func;
    }

    template<typename T, typename C>
    AsyncResult(const T& _func, const C& _callback)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, void>::value);
#endif
        wrapper_->invoke_function = _func;
        wrapper_->callback_function = _callback;
    }

    void operator()() {
        wrapper_->invoke_function();
        wrapper_->result_valid = true;

        if (wrapper_->callback_function)
            wrapper_->callback_function(true);
    }

    boost::function<void ()>& Function() { return wrapper_->invoke_function;}
    boost::function<void (bool)>& CallFunction() { return wrapper_->callback_function;}
    void Result() {}
    operator bool() const { return wrapper_->result_valid;}

  private:
    AsyncResult& operator=(const AsyncResult&);
    // AsyncResult(const AsyncResult& _ref);

  private:
    boost::shared_ptr<AsyncResultWrapper> wrapper_;
};

template <typename R>
class AsyncResult <R&> {
  private:
    struct AsyncResultWrapper {
        AsyncResultWrapper(): result_valid(false), result(NULL) {}
        ~AsyncResultWrapper() {
            if (!result_valid && callback_function)
                callback_function(*result, false);
        }

        boost::function<R& ()> invoke_function;
        boost::function<void (R&, bool)> callback_function;

        bool result_valid;
        R* result;
    };

  public:
    template<typename T>
    AsyncResult(const T& _func)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, R&>::value);
#endif
        wrapper_->invoke_function = _func;
    }

    template<typename T, typename C>
    AsyncResult(const T& _func, const C& _callback)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, R&>::value);
#endif
        wrapper_->invoke_function = _func;
        wrapper_->callback_function = _callback;
    }

    void operator()() {
        wrapper_->result = & (wrapper_->invoke_function());
        wrapper_->result_valid = true;

        if (wrapper_->callback_function)
            wrapper_->callback_function(Result(), true);
    }

    boost::function<R& ()>& Function() { return wrapper_->invoke_function;}
    boost::function<void (R&, bool)>& CallFunction() { return wrapper_->callback_function;}
    R& Result() { return *(wrapper_->result);}
    operator bool() const { return wrapper_->result_valid;}

  private:
    AsyncResult& operator=(const AsyncResult&);
    // AsyncResult(const AsyncResult& _ref);

  private:
    boost::shared_ptr<AsyncResultWrapper> wrapper_;
};

template <typename R>
class AsyncResult <const R&> {
  private:
    struct AsyncResultWrapper {
        AsyncResultWrapper(): result_valid(false), result(NULL) {}
        ~AsyncResultWrapper() {
            if (!result_valid && callback_function)
                callback_function(*result, false);
        }

        boost::function<const R& ()> invoke_function;
        boost::function<void (const R&, bool)> callback_function;

        bool result_valid;
        const R* result;
    };

  public:
    template<typename T>
    AsyncResult(const T& _func)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, const R&>::value);
#endif
        wrapper_->invoke_function = _func;
    }

    template<typename T, typename C>
    AsyncResult(const T& _func, const C& _callback)
        : wrapper_(new AsyncResultWrapper()) {
#if __cplusplus >= 201103L
        BOOST_STATIC_ASSERT(boost::is_same<typename boost::result_of<T()>::type, const R&>::value);
#endif
        wrapper_->invoke_function = _func;
        wrapper_->callback_function = _callback;
    }

    void operator()() {
        wrapper_->result = & (wrapper_->invoke_function());
        wrapper_->result_valid = true;

        if (wrapper_->callback_function)
            wrapper_->callback_function(Result(), true);
    }

    boost::function<const R& ()>& Function() { return wrapper_->invoke_function;}
    boost::function<void (const R&, bool)>& CallFunction() { return wrapper_->callback_function;}
    const R& Result() { return *(wrapper_->result);}
    operator bool() const { return wrapper_->result_valid;}

  private:
    AsyncResult& operator=(const AsyncResult&);
    // AsyncResult(const AsyncResult& _ref);

  private:
    boost::shared_ptr<AsyncResultWrapper> wrapper_;
};

template <typename R>
bool  WaitInvoke(const AsyncResult<R>& _func, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    
    if (CurrentThreadMessageQueue() == Handler2Queue(_handlerid)) {
        _func();
        return (bool)(_func);
    } else {
        WaitMessage(AsyncInvoke(_func, _handlerid));
        return (bool)(_func);
    }
}
    
template <typename F>
typename boost::result_of< F()>::type  WaitInvoke(const F& _func, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    
    if (CurrentThreadMessageQueue() == Handler2Queue(_handlerid)) {
        return _func();
    } else {
        
        typedef typename boost::result_of<F()>::type R;
        MessageQueue::AsyncResult<R> result(_func);
        
        WaitMessage(AsyncInvoke(result, _handlerid));
        return result.Result();
    }
}

template <typename R>
MessagePost_t  AsyncInvoke(const AsyncResult<R>& _func, const MessageHandler_t& _handlerid = DefAsyncInvokeHandler()) {
    return PostMessage(_handlerid, Message(0, _func));
}
    
} namespace mq = MessageQueue; //namespace MessageQueue

#endif /* MESSAGEQUEUE_H_ */