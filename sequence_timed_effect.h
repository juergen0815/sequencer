#ifndef __SEQUENCE_TIMER_EFFECT_H__
#define __SEQUENCE_TIMER_EFFECT_H__

#include "message_queue.h"
#include "sequence_effect.h"

#include <string>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class SeqTimedEffect : public SeqEffect
{
protected:
    double              m_Time;
    MessagePtr          m_Message;

    Sequencer::TimerPtr m_Timer;
public:
    SeqTimedEffect( double t, const MessagePtr& msg, int repeat = 0, const MessageQueuePtr& msgQueue = MessageQueuePtr() )
        : SeqEffect(msgQueue, repeat )
        , m_Time(t)
        , m_Message(msg)
    {
    }

    virtual ~SeqTimedEffect()
    {
        Stop();
    }

    virtual void Init( Sequencer *sequencer )
    {
        if ( sequencer ) {
        	m_Timer  = sequencer->CreateTimer();
        }
    }

    bool Start()
    {
        SeqEffect::Start();
        if ( m_Timer ) {
        	m_Timer->expires_from_now( boost::posix_time::milliseconds(m_Time));
        	m_Timer->async_wait( boost::bind(&SeqTimedEffect::Notify, this));
        }
        return false;
    }

    void Stop()
    {
        if (m_Timer) m_Timer->cancel();
        SeqEffect::Stop();
    }

protected:

    virtual void OnNotify()
    {
        std::cout << "SeqTimedEffect::OnNotify: " << m_Message->asString() << " - #" << m_RepeatCounter << std::endl;
        m_MessageQueue->Send( m_Message );
    }

};

#endif /* __SEQUENCE_TIMER_EFFECT_H__ */
