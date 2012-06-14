#ifndef __MSG_QUEUE__
#define __MSG_QUEUE__

#include <queue>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

class Message
{
public:
    Message() {}

    virtual ~Message() {}

    virtual std::string asString() const { return ""; }

    template <class T> T as() { return dynamic_cast<T>(this); }

    virtual bool operator==( const char* val )
    {
        return asString() == val;
    }

    virtual bool operator==( const Message& msg )
    {
        return asString() == msg.asString();
    }

    virtual bool operator!=( const Message& msg )
    {
        // make sure != && == are equivalent
        return ! this->operator==( msg );
    }
};
typedef boost::shared_ptr< Message > MessagePtr;

class StringMessage : public Message
{
    std::string m_Body;
public:
    StringMessage( const char* val ) : m_Body(val) {}

    virtual ~StringMessage() {}

    virtual std::string asString() const { return m_Body; }
};

template<typename T>
class MessageQueueT
{
private:
    std::queue< typename boost::shared_ptr<T> >  m_Queue;
    mutable boost::mutex      m_Lock;
    boost::condition_variable m_ConditionVar;
    bool                      m_CancelWait;
protected:
    struct compareMsgPtr
    {
        boost::shared_ptr<T> val;
        compareMsgPtr( boost::shared_ptr<T> t )  : val(t) {}
        bool operator()( const boost::shared_ptr<T>& a ) {
            bool b = *a == *val;
            return b;
        }
    };

public:
    MessageQueueT()
        : m_CancelWait(false)
    {}

    void Send( boost::shared_ptr<T> const& data)
    {
        m_Lock.lock();
        m_CancelWait = false;
        if ( data ) m_Queue.push(data);
        m_Lock.unlock();
        m_ConditionVar.notify_one();
    }

    bool IsEmpty() const
    {
        boost::mutex::scoped_lock lock(m_Lock);
        return m_Queue.empty();
    }

    void CancelWait()
    {
        m_CancelWait = true;
        m_ConditionVar.notify_one();
    }

    boost::shared_ptr<T> Poll( std::vector< typename  boost::shared_ptr<T> > mask, bool equal = true )
    {
        boost::mutex::scoped_lock lock(m_Lock);
        if (!m_Queue.empty() )
        {
            boost::shared_ptr<T> value = m_Queue.front();
            typename std::vector< boost::shared_ptr<T> >::iterator it = std::find_if( mask.begin(), mask.end(), compareMsgPtr(value));
            if ( (it != mask.end()) == equal ) {
                m_Queue.pop();
                return value;
            }
        }
        return boost::shared_ptr<T>();
    }

    boost::shared_ptr<T> Poll( )
    {
        boost::mutex::scoped_lock lock(m_Lock);
        if (m_Queue.empty() )
        {
            return boost::shared_ptr<T>();
        }

        boost::shared_ptr<T> value = m_Queue.front();
        m_Queue.pop();
        return value;
    }

    boost::shared_ptr<T> Wait( std::vector< typename boost::shared_ptr<T> > mask, bool equal = true )
    {
        do {
            boost::mutex::scoped_lock lock(m_Lock);
            while (m_Queue.empty() && !m_CancelWait)
            {
                m_ConditionVar.wait(lock);
            }
            if ( !m_CancelWait ) {
                boost::shared_ptr<T> value = m_Queue.front();
                typename std::vector< boost::shared_ptr<T> >::iterator it = std::find_if( mask.begin(), mask.end(), compareMsgPtr(value));
                if ( (it != mask.end()) == equal ) {
                    m_Queue.pop();
                    return value;
                }
            }
        } while (!m_CancelWait);

        return boost::shared_ptr<T>();
    }

    boost::shared_ptr<T> Wait()
    {
        boost::mutex::scoped_lock lock(m_Lock);
        while (m_Queue.empty() && !m_CancelWait)
        {
            m_ConditionVar.wait(lock);
        }
        if (!m_CancelWait) {
            boost::shared_ptr<T> value = m_Queue.front();
            m_Queue.pop();
            return value;
        }
        return boost::shared_ptr<T>();
    }

};

typedef MessageQueueT< Message > MessageQueue;
typedef boost::shared_ptr< MessageQueue > MessageQueuePtr;


#endif // __MSG_QUEUE__
