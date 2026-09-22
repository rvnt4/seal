#pragma once
#include "vector.h"

namespace seal
{
    template <typename... T>
    class Delegate
    {
    public:
        using Stub = void (*)(void*, T...);
        void* context;
        Stub stub;

        Delegate() : context(nullptr), stub(nullptr) {}
        Delegate(void* ctx, Stub s) : context(ctx), stub(s) {}

        void operator()(T... args) const
        {
            if (stub) stub(context, static_cast<T&&>(args)...);
        }
        
        bool operator==(const Delegate& other) const { return context == other.context && stub == other.stub; }
        bool operator!=(const Delegate& other) const { return !(*this == other); }
    };

    template <typename... T> 
    class Event
    {
    public:
        using ConnectionId = usize;

        explicit Event(IAllocator* alloc) : _listeners(alloc), _nextId(0) {}

        ConnectionId addListener(Delegate<T...> listener)
        {
            ConnectionId id = ++_nextId;
            _listeners.push_back({id, listener});
            return id;
        }

        bool removeListener(ConnectionId id)
        {
            for (usize i = 0; i < _listeners.size(); ++i)
            {
                if (_listeners[i].id == id)
                {
                    _listeners.erase(i);
                    return true;
                }
            }
            return false;
        }

        void run(T... args)
        {
            for (usize i = 0; i < _listeners.size(); ++i)
                _listeners[i].listener(...);
        }

        void clear()
        {
            _listeners.clear();
        }

    private:
        struct ListenerInfo
        {
            ConnectionId id;
            Delegate<T...> listener;
        };

        Vector<ListenerInfo> _listeners;
        ConnectionId _nextId;
    };
} // namespace seal
