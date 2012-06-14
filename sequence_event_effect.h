#ifndef __SEQUENCE_EVENT_EFFECT_H__
#define __SEQUENCE_EVENT_EFFECT_H__

#include "message_queue.h"
#include "listener.h"
#include "sequence_effect.h"

#include <string>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class SeqEventEffect : public SeqEffect
{
protected:
    MessagePtr    m_MessageToListen;
    MessagePtr    m_Message;

public:
    SeqEventEffect( const MessagePtr& listen, const MessagePtr& msg, const MessageQueuePtr& msgQueue = MessageQueuePtr() )
        : SeqEffect(msgQueue)
        , m_MessageToListen(listen)
        , m_Message(msg)
    {
    }

    virtual ~SeqEventEffect()
    {
    }

    virtual void Init( Sequencer *sequencer )
    {
        if ( sequencer ) {
            GenericEventListener::OnMessageFunc onEvent( boost::bind( &SeqEventEffect::OnEvent, this, _1 ) );
            sequencer->RegisterListener( ListenerPtr( new GenericEventListener( m_MessageToListen, onEvent) ));
        }
    }

    virtual bool Start()
    {
        return true;
    }

    virtual void Stop()
    {
        SeqEffect::Stop();
    }

protected:
    virtual bool OnEvent( const MessagePtr& msg )
    {
        Notify();
        return true;
    }

    virtual void OnNotify()
    {
        std::cout << "SeqEventEffect::OnNotify: " << m_Message->asString() << std::endl;
        m_MessageQueue->Send( m_Message );
    }

};


#endif /* __SEQUENCE_EVENT_EFFECT_H__ */
