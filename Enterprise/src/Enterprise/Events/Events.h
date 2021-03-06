#pragma once
#include "EP_PCH.h"
#include "Core.h"

namespace Enterprise
{

/// The Enterprise events system.
/// @see @ref Events
class Events
{
public:
    
    /// An Enterprise event.
    class Event
    {
    public:
        /// Constructor.
        /// @param type This event's type.
        Event(HashName type) : m_type(type) {}

        /// Get this event's type.
        /// @return This event's type.
        inline const HashName Type() { return m_type; }
        
        Event() = delete;
        virtual ~Event() {}

    private:
        HashName m_type;
    };
    
    
    /// An Enterprise event with a data payload.
    /// @tparam T The type of the data payload.
    template <typename T>
    class DataEvent : public Event
    {
    public:
        /// Constructor.
        /// @param type This event's type.
        /// @param data The data payload.
        DataEvent(HashName type, T data) : m_data(data), Event(type) {}

        /// Get this event's data payload.
        /// @return The data payload.
        T& Data() { return m_data; }

        DataEvent() = delete;
        
    private:
        T m_data;
    };
    

    /// A pointer to an event callback function.
    typedef bool(*EventCallbackPtr)(Event&);


    /// Register a callback for an event type.
    /// @param type The HashName of the event type.
    /// @param callback A pointer to the callback function.
    static void Subscribe(HashName type, EventCallbackPtr callback);

    /// Register a callback for multiple event types at once.
    /// @param types A list of HashNames for event types to subscribe to.
    /// @param callback A pointer to the callback function.
    static void Subscribe(std::initializer_list<HashName> types, EventCallbackPtr callback);

    
    /// Dispatch a pre-made event.
    /// @param e A reference to the event to dispatch.
    static void Dispatch(Event& e);

    /// Dispatch a new event of the given type.
    /// @param type The desired event type.
    static void Dispatch(HashName type);

    /// Dispatch a new event with a data payload.
    /// @tparam T Type of the data payload.
    /// @param type The desired event type.
    /// @param data The data payload.
    template <typename T>
    static void Dispatch(HashName type, T data)
    {
        // Generate the event
        Events::DataEvent<T> e = {type, data};
        
        // Call each registered callback until one returns true
        for (auto callbackit = callbackPtrs[type].rbegin();
                  callbackit != callbackPtrs[type].rend();
                  ++callbackit)
        {
            if ((*callbackit)(e))
                break;
        }
    }


    /// Extract the data payload from a DataEvent.
    /// @tparam T The type of the data payload.
    /// @param e Reference to the event to unpack.
    /// @return The data payload.
    /// @note C++17's structured bindings are useful for extracting data from tuples, as in:
    /// @code auto[x, y] \= Events\::Unpack&lt;std\::pair&lt;int, int&gt;&gt;(e); @endcode
    template <typename T>
    static T& Unpack(Enterprise::Events::Event& e) 
    {
        Enterprise::Events::DataEvent<T>* converted = dynamic_cast<Enterprise::Events::DataEvent<T>*>(&e);
        EP_ASSERTF(converted, "Events: Unpack() cannot cast event to requested DataEvent type.");
        return converted->Data();
    }

    
private:

    /// Hash table of callback pointer vectors.  Key is the HashName of the event type.
    /// @note Callbacks are invoked in FILO order.
    static std::unordered_map<HashName, std::vector<Events::EventCallbackPtr>> callbackPtrs;

};

}
