/*
 * main.cpp
 *
 *  Created on: Apr 19, 2012
 *      Author: jschober
 */

#include "message_queue.h"
#include "listener.h"
#include "sequencer.h"
#include "sequence_timed_effect.h"
#include "sequence_event_effect.h"

#include <string>
#include <iostream>

/* Example for a custom listener within the application.
 * This listener listens for a hard coded "STOPPED" message.
 * The Sequencers sends a "STOPPED" message after it received a "TERMINATE"
 */
class AppListener : public Listener
{
    // use our own message queue
    MessageQueuePtr m_MessageQueue;
public:
    AppListener() : m_MessageQueue( new MessageQueue )
    {
    }

    // called from sequencer message thread
    virtual bool OnEvent( const MessagePtr& msg )
    {
        if ( msg->asString() == "STOPPED" ) { m_MessageQueue->Send( msg ); return true; }
        return false;
    }

    // called from main thread
    void WaitMessage()
    {
        bool terminate(false);
        do {
            // do not process message in mask - normally we would only list to TERMINATE here.
            MessagePtr evt = m_MessageQueue->Wait( );
            while ( !terminate && evt != NULL ) {
                terminate = ( evt->asString().compare( "STOPPED" ) == 0 );
                std::cout << "AppListener::Received Event: " << evt->asString() << std::endl;
                if ( !terminate) evt = m_MessageQueue->Poll( );
            }
        } while ( !terminate );
        std::cout << "AppListener::Exit" << std::endl;
    }
};
// Create a typedef to safe us a cast (see below)
typedef boost::shared_ptr<AppListener> AppListenerPtr;

int main( int argc, char* argv[] )
{
    // simple sequence- almost like a timed statemachine; does not have a state transition on its own, though
    Sequencer sequencer;
    // repeat 5 times every 200ms
    sequencer.Add( SeqEffectPtr( new SeqTimedEffect( 200, MessagePtr( new StringMessage("TwoHundret")), 5 )));
    // We send this message and also use it later to listen to it
    MessagePtr middle( new StringMessage("Middle!"));
    // one shot @ 500ms.
    sequencer.Add( SeqEffectPtr( new SeqTimedEffect( 500, middle )));
    // exit after 1 second, we could also use a counter in "OneHundret" and send terminate from there. Add 10ms to avoid overlap with 1
    sequencer.Add( SeqEffectPtr( new SeqTimedEffect( 1010, MessagePtr( new Sequencer::StateMessage( Sequencer::TERMINATE)) )));
    // this one waits for an event, send from another effect (middle)
    sequencer.Add( SeqEffectPtr( new SeqEventEffect( middle,  MessagePtr( new StringMessage("Hey, we got a message! Creating a new one!")) )));
    // Start it
    sequencer.Start();

    // The application also adds a listener and listens to STOPPED events
    AppListenerPtr appListener( new AppListener() );
    sequencer.RegisterListener( appListener );
    // and we wait until the sequencer sends a STOPPED message
    appListener->WaitMessage();

    std::cout << "Application::Exit" << std::endl;

    return EXIT_SUCCESS;
}
