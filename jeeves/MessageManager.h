#ifndef MessageManager_h
#define MessageManager_h

#include <Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "sys_fixed_single.h"
#include "configuration.h"

// Size of the message two queues.  Adjust as appropriate for your application.
// Requires a total of 4 * sizeof(int) bytes of RAM for each unit of size
#ifndef MESSAGEMANAGER_MESSAGE_QUEUE_SIZE
#define MESSAGEMANAGER_MESSAGE_QUEUE_SIZE		8
#endif

//#define MESSAGEMANAGER_DEBUG  true

class MessageManager
{

public:

    // Type for an message listener (a.k.a. callback) function
    typedef void ( *MessageListener )( String messageString, int messageDuration );

    // MessageManager recognizes two kinds of messages.  By default, messages are
    // are queued as low priority, but these constants can be used to explicitly
    // set the priority when queueing messages
    //
    // NOTE high priority messages are always handled before any low priority messages.
    enum MessagePriority { kHighPriority, kLowPriority };

    // Create a message manager
    // It always operates in interrupt safe mode, allowing you to queue messages from interrupt handlers
    MessageManager();

    // Returns true if no messages are in the queue
    boolean isMessageQueueEmpty( MessagePriority pri = kLowPriority );

    // Returns true if no more messages can be inserted into the queue
    boolean isMessageQueueFull( MessagePriority pri = kLowPriority );

    // Actual number of messages in queue
    int getNumMessagesInQueue( MessagePriority pri = kLowPriority );

    // tries to insert a message into the queue;
    // returns true if successful, false if the
    // queue if full and the message cannot be inserted
    boolean queueMessage( String messageString, int messageDuration, MessagePriority pri = kLowPriority, bool invert = false );

    // this must be called regularly (usually by calling it inside the loop() function)
    boolean processMessage();

    // this function can be called to process ALL messages in the queue
    // WARNING:  if interrupts are adding messages as fast as they are being processed
    // this function might never return.  YOU HAVE BEEN WARNED.
    int processAllMessages();


private:

    // MessageQueue class used internally by MessageManager
    class MessageQueue
    {

    public:

        // Queue constructor
        MessageQueue();

        // Returns true if no messages are in the queue
        boolean isEmpty();

        // Returns true if no more messages can be inserted into the queue
        boolean isFull();

        // Actual number of messages in queue
        int getNumMessages();

        // Tries to insert a message into the queue;
        // Returns true if successful, false if the queue if full and the message cannot be inserted
        //
        // NOTE: if MessageManager is instantiated in interrupt safe mode, this function can be called
        // from interrupt handlers.  This is the ONLY MessageManager function that can be called from
        // an interrupt.
        boolean queueMessage( String messageString, int messageDuration, bool invert = false );

        // Tries to extract an message from the queue;
        // Returns true if successful, false if the queue is empty (the parameteres are not touched in this case)
        boolean popMessage( String* messageString, int* messageDuration, bool* invert );

    private:

        // Message queue size.
        // The maximum number of messages the queue can hold is kMessageQueueSize
        // Increasing this number will consume 2 * sizeof(int) bytes of RAM for each unit.
        static const int kMessageQueueSize = MESSAGEMANAGER_MESSAGE_QUEUE_SIZE;

        struct MessageElement
        {
            String body;	// each message is represented by a String
            int duration;	// each message has a single integer duration
            bool invert;   // each message can be displayed inverted
        };

        // The message queue
        MessageElement mMessageQueue[ kMessageQueueSize ];

        // Index of message queue head
        int mMessageQueueHead;

        // Index of message queue tail
        int mMessageQueueTail;

        // Actual number of messages in queue
        int mNumMessages;

    };

    void printText(uint8_t modStart, uint8_t modEnd, char *pMsg, bool invert = false);
    void printText(uint8_t modStart, uint8_t modEnd, String pMsg, bool invert = false);
    void showTime();

    MessageQueue 	mHighPriorityQueue;
    MessageQueue 	mLowPriorityQueue;

    // millis last message displayed.
    int mMillisNextMessageAllowed;
    int mPreviousEpoch;
    char scrollingMessageBuffer[1000];

    MD_Parola P = MD_Parola(HARDWARE_TYPE, LED_MATRIX_CS_PIN, MAX_DEVICES);

};

//*********  INLINES   MessageManager::  ***********

inline boolean MessageManager::isMessageQueueEmpty( MessagePriority pri )
{
    return ( pri == kHighPriority ) ? mHighPriorityQueue.isEmpty() : mLowPriorityQueue.isEmpty();
}

inline boolean MessageManager::isMessageQueueFull( MessagePriority pri )
{
    return ( pri == kHighPriority ) ? mHighPriorityQueue.isFull() : mLowPriorityQueue.isFull();
}

inline int MessageManager::getNumMessagesInQueue( MessagePriority pri )
{
    return ( pri == kHighPriority ) ? mHighPriorityQueue.getNumMessages() : mLowPriorityQueue.getNumMessages();
}

inline boolean MessageManager::queueMessage( String messageString, int messageDuration, MessagePriority pri, bool invert )
{
    return ( pri == kHighPriority ) ?
        mHighPriorityQueue.queueMessage( messageString, messageDuration, invert ) : mLowPriorityQueue.queueMessage( messageString, messageDuration, invert );
}




//*********  INLINES   MessageManager::MessageQueue::  ***********

inline boolean MessageManager::MessageQueue::isEmpty()
{
    return ( mNumMessages == 0 );
}


inline boolean MessageManager::MessageQueue::isFull()
{
    return ( mNumMessages == kMessageQueueSize );
}


inline int MessageManager::MessageQueue::getNumMessages()
{
    return mNumMessages;
}

#endif
