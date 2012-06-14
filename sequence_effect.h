#ifndef __SEQUENCE_EFFECT_H__
#define __SEQUENCE_EFFECT_H__

#include "message_queue.h"

#include <string>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



class SeqEffect;
typedef boost::shared_ptr< SeqEffect > SeqEffectPtr;

class Sequencer;

class SeqEffect
{
protected:
    MessageQueuePtr m_MessageQueue;
    int             m_RepeatCounter;

public:
    SeqEffect( int repeat = 0 ) : m_RepeatCounter(repeat) {}

    SeqEffect( const MessageQueuePtr& msgQueue, int repeat = 0 )
        : m_MessageQueue(msgQueue)
        , m_RepeatCounter(repeat)
    {
    }

    virtual ~SeqEffect()
    {
        Stop();
    }

    virtual void Init( Sequencer *sequencer ) {}

    virtual void SetMessageQueue(const MessageQueuePtr& msgQueue)
    {
        m_MessageQueue = msgQueue;
    }

    MessageQueuePtr GetMessageQueue() const
    {
        return m_MessageQueue;
    }

    virtual bool Start()
    {
        return true;
    }

    virtual void Stop()
    {
    }

protected:
    virtual void Notify()
    {
        OnNotify();
        if ( m_RepeatCounter < 0 ) {
            Start();
        } else if (m_RepeatCounter > 1 ) {
            --m_RepeatCounter;
            Start();
        } else {
            Stop();
        }
    }

    virtual void OnNotify() {}
};


#endif /* __SEQUENCE_EFFECT_H__ */
