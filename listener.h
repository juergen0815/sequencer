/*
 * listener.h
 *
 *  Created on: Apr 23, 2012
 *      Author: jschober
 */

#ifndef __LISTENER_H__
#define __LISTENER_H__

#include "message_queue.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>

class Listener;
typedef boost::shared_ptr<Listener> ListenerPtr;


class Listener
{
public:
    Listener() {}

    virtual ~Listener() {}

    virtual bool OnEvent( const MessagePtr& msg ) { return false; }
};

class GenericListener : public Listener
{
protected:
    GenericListener();
public:
    typedef boost::function< bool ( const MessagePtr& msg ) > OnMessageFunc;

    GenericListener( const OnMessageFunc& func ) : m_OnMessageFunc(func) { }

    virtual ~GenericListener() {}

    virtual bool OnEvent( const MessagePtr& msg ) { return m_OnMessageFunc( msg ); }
protected:
    OnMessageFunc m_OnMessageFunc;
};

class GenericEventListener : public GenericListener
{
protected:
    GenericEventListener();

public:
    GenericEventListener( const MessagePtr& mask , const OnMessageFunc& func ) : GenericListener(func), m_EventMask(mask) {}

    virtual ~GenericEventListener() {}

    const MessagePtr& GetEventMask() const { return m_EventMask; }

    virtual bool OnEvent( const MessagePtr& msg ) {
        return ( *msg == *m_EventMask ) ? m_OnMessageFunc( msg ) : false;
    }
protected:
    MessagePtr m_EventMask;
};

class EventListener : public Listener
{
    // use our own message queue
    MessageQueuePtr m_MessageQueue;
    MessagePtr      m_ListenEvent;

    EventListener( const EventListener& );
public:
    EventListener() : m_MessageQueue( new MessageQueue )
    {
    }

    EventListener( const MessagePtr& evt ) : m_MessageQueue( new MessageQueue ), m_ListenEvent(evt)
    {
    }

    void SetEventToListen(const MessagePtr& evt)
    {
        m_ListenEvent = evt;
    }

    const MessagePtr& GetEventToListen() const
    {
        return m_ListenEvent;
    }
    // called from main thread
    void WaitEvent()
    {
        bool terminate(false);
        do {
            // do not process message in mask - normally we would only list to TERMINATE here.
            MessagePtr evt = m_MessageQueue->Wait( );
            while ( !terminate && evt != NULL ) {
                terminate = ( *evt == *m_ListenEvent );
                if ( !terminate) evt = m_MessageQueue->Poll( );
            }
        } while ( !terminate );
    }

    bool PollEvent()
    {
        MessagePtr evt = m_MessageQueue->Poll();
        if ( evt ) {
            return ( *evt == *m_ListenEvent );
        }
        return false;
    }
protected:
    // called from sequencer message thread
    virtual bool OnEvent( const MessagePtr& msg )
    {
        if ( *msg == *m_ListenEvent ) { m_MessageQueue->Send( msg ); return true; }
        return false;
    }

};


#endif /* __LISTENER_H__ */
