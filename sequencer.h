/*
 * sequencer.h
 *
 *  Created on: Apr 19, 2012
 *      Author: jschober
 */

#ifndef __SEQUENCER_H__
#define __SEQUENCER_H__

#include "sequence_effect.h"
#include "listener.h"

#include <vector>
#include <list>

#include <boost/thread/detail/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class Sequencer;
typedef boost::shared_ptr<Sequencer> SequencerPtr;

class Sequencer
{
public:
    enum SeqStates
    {
        IDLE,
        START,
        RUNNING,
        TERMINATE,
        STOPPED,
        SUSPEND,
        RESUME
    };

    class StateMessage : public Message
    {
        SeqStates m_Body;
    public:
        StateMessage( SeqStates state ) : m_Body(state) {}

        virtual ~StateMessage() {}

        virtual std::string asString() const
        {
            switch (m_Body) {
            case IDLE:      return "IDLE";
            case START:     return "START";
            case RUNNING:   return "RUNNING";
            case TERMINATE: return "TERMINATE";
            case STOPPED:   return "STOPPED";
            case SUSPEND:   return "SUSPEND";
            case RESUME:    return "RESUME";
            default: break;
            }
            return "";
        }

        SeqStates GetState() const { return m_Body; }

        bool operator==( SeqStates state )
        {
            return m_Body == state;
        }
    };

    mutable boost::mutex                m_ListenerLock;

    typedef boost::asio::deadline_timer Timer;
    typedef boost::shared_ptr<Timer>    TimerPtr;

protected:
    SeqStates                           m_SeqState;
    MessageQueuePtr                     m_MessageQueue;
    std::vector< SeqEffectPtr >         m_SeqEvents;

    bool                                m_TerminateThread;
    boost::shared_ptr< boost::thread >  m_MsgThread;

    std::vector< MessagePtr >           m_ListenerMask;

    typedef std::list< ListenerPtr > ListenerList;
    ListenerList                        m_ListerList;

    boost::asio::io_service             m_IOService;
    boost::asio::io_service::work       m_IOServiceWork;

    boost::shared_ptr< boost::thread >  m_ServiceThread;
public:
    Sequencer( const MessageQueuePtr& msgQueue )
        : m_SeqState(IDLE)
        , m_MessageQueue( msgQueue )
        , m_MsgThread( new boost::thread( boost::bind( &Sequencer::MessageThread, this )))
    	, m_IOServiceWork(m_IOService)
    	, m_ServiceThread( new boost::thread( boost::bind( &boost::asio::io_service::run, &m_IOService )))
    {
    }

    Sequencer( )
        : m_SeqState(IDLE)
        , m_MessageQueue( new MessageQueue() )
        , m_MsgThread( new boost::thread( boost::bind( &Sequencer::MessageThread, this )))
		, m_IOServiceWork(m_IOService)
    	, m_ServiceThread( new boost::thread( boost::bind( &boost::asio::io_service::run, &m_IOService )))
    {
    }

    virtual ~Sequencer()
    {
        Stop();
        m_IOService.stop();
        m_ServiceThread->join();

        // send terminate to thread
        m_MessageQueue->CancelWait();
        m_MsgThread->join();

        boost::mutex::scoped_lock lock(m_ListenerLock);
        m_ListerList.clear();
    }

    TimerPtr CreateTimer()
    {
    	return TimerPtr(new Timer(m_IOService));
    }

    ListenerPtr RegisterListener( const ListenerPtr& listener )
    {
        boost::mutex::scoped_lock lock(m_ListenerLock);

        m_ListerList.push_back( listener );
        return listener;
    }

    void UnregisterListener( const ListenerPtr& listener )
    {
        boost::mutex::scoped_lock lock(m_ListenerLock);
        m_ListerList.remove_if( boost::bind( &Sequencer::IsListener, this, _1, listener )  );
    }

    void SetMessageQueue( const MessageQueuePtr& msgQueue )
    {
        m_MessageQueue = msgQueue;
    }

    MessageQueuePtr GetMessageQueue() const
    {
        return m_MessageQueue;
    }

    SeqEffectPtr Add( SeqEffectPtr t )
    {
        if ( t && t->GetMessageQueue() == NULL ) {
            t->SetMessageQueue( m_MessageQueue );
            t->Init( this );
        }
        m_SeqEvents.push_back( t );
        return t;
    }

    void Clear()
    {
        Stop();
        // interrupt message handling
        m_MessageQueue->CancelWait();
        m_SeqEvents.clear();
        //
    }

    void Start()
    {
        if ( m_SeqState != START ) {
            m_MessageQueue->Send( MessagePtr( new StateMessage(START) ));
        }
    }

    void Stop()
    {
        if ( m_SeqState!= STOPPED ) {
            m_MessageQueue->Send( MessagePtr( new StateMessage(STOPPED) ));
        }
    }

protected:
    bool IsListener( const ListenerPtr& a, const ListenerPtr& b ) {
        return a.get() == b.get();
    }

    virtual void OnStart()
    {
        std::for_each( m_SeqEvents.begin(), m_SeqEvents.end(), boost::bind( &SeqEffect::Start, _1 ) );
    }

    virtual void OnStop()
    {
        std::for_each( m_SeqEvents.begin(), m_SeqEvents.end(), boost::bind( &SeqEffect::Stop, _1 ) );
        OnProcessEvent( MessagePtr( new StateMessage(STOPPED) ) );
    }

    virtual void OnSuspend()
    {
        // TODO: Notify effects that sequencer got suspended - e.g. stop timers, etc.
    }

    virtual void OnResume()
    {
        // TODO: Notify effects that sequencer resumes, e.g. restart timers, etc.
    }

    virtual void OnProcessEvent( const MessagePtr& evt )
    {
        boost::mutex::scoped_lock lock(m_ListenerLock);
        if ( m_ListerList.size() > 0 ) {
            MessagePtr stopEvt( new StateMessage(m_SeqState) );
            std::for_each( m_ListerList.begin(), m_ListerList.end(), boost::bind( &Listener::OnEvent, _1, evt ));
        }
    }

    virtual void MessageThread()
    {
        bool terminate(false);

        do {
            MessagePtr evt = m_MessageQueue->Wait();
            if (evt == NULL) break; // got canceled
            while ( !terminate && evt ) {
                std::cout << "Sequencer::Received: " << evt->asString() << std::endl;
                StateMessage* sm = evt ? evt->as<StateMessage*>() : NULL;
                if ( sm ) {
                    switch (sm->GetState()) {
                    case TERMINATE:
                        terminate = true;
                        m_SeqState = STOPPED;
                        break;
                    case START:
                        m_SeqState = START;
                        break;
                        // IDLE == Suspend
                    case SUSPEND:
                        if ( m_SeqState == RUNNING ) {
                            m_SeqState = SUSPEND;
                        }
                        break;
                        // RUNNING == RESUME
                    case RESUME:
                        if ( m_SeqState == IDLE ) {
                            m_SeqState = RESUME;
                        }
                        break;
                    case STOPPED:
                        m_SeqState = STOPPED;
                        break;
                    default:
                        break;
                    }
                }
                switch ( m_SeqState ) {
                case START:
                    OnStart();
                    m_SeqState = RUNNING;
                    break;
                case SUSPEND:
                    OnSuspend();
                    m_SeqState = IDLE;
                    break;
                    // RUNNING == RESUME
                case RESUME:
                    OnResume();
                    m_SeqState = RUNNING;
                    break;
                case RUNNING:
                    OnProcessEvent(evt);
                    break;
                case STOPPED:
                    OnStop();
                    m_SeqState = IDLE;
                    break;
                default:
                    break;
                }

                if (!terminate) evt = m_MessageQueue->Poll( );
            }
        } while (!terminate);

    }

};


#endif /* __SEQUENCER_H__ */
