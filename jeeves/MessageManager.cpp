#include "MessageManager.h"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "sys_fixed_single.h"
#include "configuration.h"
#include "NTPClient.h"

extern NTPClient timeClient;

namespace
{
    // This class takes care of turning interrupts on and off.
    class SuppressInterrupts
    {
    public:

        // Record the current state and suppress interrupts when the object is instantiated.
        SuppressInterrupts()
        {
            // This turns off interrupts and gets the old state in one function call
            // See https://github.com/esp8266/Arduino/issues/615 for details
            // level 15 will disable ALL interrupts,
            // level 0 will enable ALL interrupts
            mSavedInterruptState = xt_rsil( 15 );
        }

        // Restore whatever interrupt state was active before
        ~SuppressInterrupts()
        {
            // Restore the old interrupt state
            xt_wsr_ps( mSavedInterruptState );
        }

    private:

        uint32_t    mSavedInterruptState;
    };

}

#if MESSAGEMANAGER_DEBUG
#define MSGMGR_DEBUG_PRINT( x )		Serial.print( x );
#define MSGMGR_DEBUG_PRINTLN( x )	Serial.println( x );
#define MSGMGR_DEBUG_PRINT_PTR( x )	Serial.print( reinterpret_cast<unsigned long>( x ), HEX );
#define MSGMGR_DEBUG_PRINTLN_PTR( x )	Serial.println( reinterpret_cast<unsigned long>( x ), HEX );
#else
#define MSGMGR_DEBUG_PRINT( x )
#define MSGMGR_DEBUG_PRINTLN( x )
#define MSGMGR_DEBUG_PRINT_PTR( x )
#define MSGMGR_DEBUG_PRINTLN_PTR( x )
#endif


MessageManager::MessageManager()
{
    
    P.begin();
    P.displayText("Jeeves v1.4.0 - M1DST", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();
    scrollingMessageBuffer[0] = 0;
}


boolean MessageManager::processMessage()
{
    P.displayAnimate();

    String messageString;
    int duration;
    bool invert;
    boolean handled = false;

    if(millis() >= mMillisNextMessageAllowed)
    {

        if ( mHighPriorityQueue.popMessage( &messageString, &duration, &invert ) )
        {

            mMillisNextMessageAllowed = millis() + duration;
            printText(0, MAX_DEVICES-1, messageString, invert);
            handled = true;

            MSGMGR_DEBUG_PRINT( "processMessage() hi-pri message : " )
            MSGMGR_DEBUG_PRINT( messageString )
            MSGMGR_DEBUG_PRINT( ", " )
            MSGMGR_DEBUG_PRINTLN( duration )
        }

        // If no high-pri messages handled (because there are no high-pri messages) then try low-pri messages
        if ( !handled && mLowPriorityQueue.popMessage( &messageString, &duration, &invert ) )
        {

            mMillisNextMessageAllowed = millis() + duration;
            printText(0, MAX_DEVICES-1, messageString, invert);
            handled = true;

            MSGMGR_DEBUG_PRINT( "processMessage() lo-pri message : " )
            MSGMGR_DEBUG_PRINT( messageString )
            MSGMGR_DEBUG_PRINT( ", " )
            MSGMGR_DEBUG_PRINTLN( duration )
        }

        // No messages to show so we will show the time.
        if ( !handled )
        {

            showTime();
            handled = true;

        }
        

    }

    return handled;
}


int MessageManager::processAllMessages()
{
    P.displayAnimate();

    String messageString;
    int duration;
    bool invert;
    int handledCount = 0;

    while ( mHighPriorityQueue.popMessage( &messageString, &duration, &invert ) )
    {
        mMillisNextMessageAllowed = millis() + duration;
        printText(0, MAX_DEVICES-1, messageString, invert);
        handledCount ++;

        MSGMGR_DEBUG_PRINT( "processMessage() hi-pri message " )
        MSGMGR_DEBUG_PRINT( messageString )
        MSGMGR_DEBUG_PRINT( ", " )
        MSGMGR_DEBUG_PRINT( duration )
        MSGMGR_DEBUG_PRINT( " sent to " )
        MSGMGR_DEBUG_PRINTLN( handledCount )
    }

    while ( mLowPriorityQueue.popMessage( &messageString, &duration, &invert ) )
    {
        mMillisNextMessageAllowed = millis() + duration;
        printText(0, MAX_DEVICES-1, messageString, invert);
        handledCount ++;

        MSGMGR_DEBUG_PRINT( "processMessage() lo-pri message " )
        MSGMGR_DEBUG_PRINT( messageString )
        MSGMGR_DEBUG_PRINT( ", " )
        MSGMGR_DEBUG_PRINT( duration )
        MSGMGR_DEBUG_PRINT( " sent to " )
        MSGMGR_DEBUG_PRINTLN( handledCount )
    }

    return handledCount;
}



/********************************************************************/

MessageManager::MessageQueue::MessageQueue() :
mMessageQueueHead( 0 ),
mMessageQueueTail( 0 ),
mNumMessages( 0 )
{
    for ( int i = 0; i < kMessageQueueSize; i++ )
    {
        mMessageQueue[i].body = "";
        mMessageQueue[i].duration = 0;
    }
}



boolean MessageManager::MessageQueue::queueMessage( String messageString, int messageDuration, bool invert )
{
    /*
    * The call to noInterrupts() MUST come BEFORE the full queue check.
    *
    * If the call to isFull() returns FALSE but an asynchronous interrupt queues
    * an message, making the queue full, before we finish inserting here, we will then
    * corrupt the queue (we'll add an message to an already full queue). So the entire
    * operation, from the call to isFull() to completing the inserting (if not full)
    * must be atomic.
    *
    * Note that this race condition can only arise IF both interrupt and non-interrupt (normal)
    * code add messages to the queue.  If only normal code adds messages, this can't happen
    * because then there are no asynchronous additions to the queue.  If only interrupt
    * handlers add messages to the queue, this can't happen because further interrupts are
    * blocked while an interrupt handler is executing.  This race condition can only happen
    * when an message is added to the queue by normal (non-interrupt) code and simultaneously
    * an interrupt handler tries to add an message to the queue.  This is the case that the
    * cli() (= noInterrupts()) call protects against.
    *
    * Contrast this with the logic in popMessage().
    *
    */

    SuppressInterrupts  interruptsOff;      // Interrupts automatically restored when exit block

    // ATOMIC BLOCK BEGIN
    boolean retVal = false;
    if ( !isFull() )
    {
        // Store the message at the tail of the queue
        mMessageQueue[ mMessageQueueTail ].body = messageString;
        mMessageQueue[ mMessageQueueTail ].duration = messageDuration;
        mMessageQueue[ mMessageQueueTail ].invert = invert;

        // Update queue tail value
        mMessageQueueTail = ( mMessageQueueTail + 1 ) % kMessageQueueSize;;

        // Update number of messages in queue
        mNumMessages++;

        retVal = true;
    }
    // ATOMIC BLOCK END

    //Serial.println(retVal);

    return retVal;
}


boolean MessageManager::MessageQueue::popMessage( String* messageString, int* messageDuration, bool* invert )
{
    /*
    * The call to noInterrupts() MUST come AFTER the empty queue check.
    *
    * There is no harm if the isEmpty() call returns an "incorrect" TRUE response because
    * an asynchronous interrupt queued an message after isEmpty() was called but before the
    * return is executed.  We'll pick up that asynchronously queued message the next time
    * popMessage() is called.
    *
    * If interrupts are suppressed before the isEmpty() check, we pretty much lock-up the Arduino.
    * This is because popMessage(), via processMessages(), is normally called inside loop(), which
    * means it is called VERY OFTEN.  Most of the time (>99%), the message queue will be empty.
    * But that means that we'll have interrupts turned off for a significant fraction of the
    * time.  We don't want to do that.  We only want interrupts turned off when we are
    * actually manipulating the queue.
    *
    * Contrast this with the logic in queueMessage().
    *
    */

    if ( isEmpty() )
    {
        return false;
    }

    SuppressInterrupts  interruptsOff;      // Interrupts automatically restored when exit block

    // Pop the message from the head of the queue
    // Store message and message duration into the user-supplied variables
    *messageString  = mMessageQueue[ mMessageQueueHead ].body;
    *messageDuration = mMessageQueue[ mMessageQueueHead ].duration;
    *invert = mMessageQueue[ mMessageQueueHead ].invert;

    // Clear the message (paranoia)
    mMessageQueue[ mMessageQueueHead ].body = "";

    // Update the queue head value
    mMessageQueueHead = ( mMessageQueueHead + 1 ) % kMessageQueueSize;

    // Update number of messages in queue
    mNumMessages--;

    return true;
}


void MessageManager::showTime() {
  if (timeClient.getEpochTime() > 1535224313) {
    if (timeClient.getEpochTime() > mPreviousEpoch) {
      mPreviousEpoch = timeClient.getEpochTime();
      String currentTimeMessage = "TIME: ";
      if (timeClient.getEpochTime() % 2 == 0) {
        String s = timeClient.getFormattedTime();
        currentTimeMessage += s;
      } else {
        currentTimeMessage += timeClient.getFormattedTime();
      }
      currentTimeMessage += " UTC";
      P.setFont(_sys_fixed_single);
      MSGMGR_DEBUG_PRINTLN( currentTimeMessage );
      printText(0, MAX_DEVICES - 1, currentTimeMessage);
      P.setFont(0);
    }
  } else {
    MSGMGR_DEBUG_PRINTLN( "Waiting for NTP." );
    printText(0, MAX_DEVICES - 1, "* * Waiting for NTP! * *");
  }
}

void MessageManager::printText(uint8_t modStart, uint8_t modEnd, String pMsg, bool invert)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
    char message [pMsg.length() + 1];
    pMsg.toCharArray(message, sizeof(message));
    printText(modStart, modEnd, message, invert);
}

void MessageManager::printText(uint8_t modStart, uint8_t modEnd, char *pMsg, bool invert)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
    P.displayReset();
    P.setInvert(invert);

    if(strlen(pMsg) > 24)
    {
        strcpy(scrollingMessageBuffer, pMsg);
        P.displayClear();
        P.displayText(scrollingMessageBuffer, PA_LEFT, 35, 2000, PA_SCROLL_LEFT, PA_NO_EFFECT);
    }
    else
    {
        P.displayText(pMsg, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        P.displayAnimate();
    }
}