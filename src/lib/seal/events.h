#pragma once
#include "vector.h"

namespace seal
{
    template <typename... T> class Delegate
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

    template <typename... T> class Event
    {
        public:
            using ConnectionId = usize;
            static constexpr ConnectionId InvalidConnection = 0;

            explicit Event(IAllocator* alloc)
                : _listeners(alloc), _nextId(0), _dispatching(false), _pendingRemovals(false)
            {
            }

            ConnectionId addListener(Delegate<T...> listener)
            {
                ConnectionId id = ++_nextId;
                if (id == InvalidConnection) id = ++_nextId;

                ListenerInfo info{id, listener};
                if (!_listeners.push_back(static_cast<ListenerInfo&&>(info))) return InvalidConnection;
                return id;
            }

            bool removeListener(ConnectionId id)
            {
                if (id == InvalidConnection) return false;
                for (usize i = 0; i < _listeners.size(); ++i)
                {
                    if (_listeners[i].id == id && _listeners[i].listener.stub)
                    {
                        if (_dispatching)
                        {
                            _listeners[i].listener.stub = nullptr;
                            _pendingRemovals = true;
                        }
                        else
                        {
                            _listeners.erase(i);
                        }
                        return true;
                    }
                }
                return false;
            }

            void run(const T&... args)
            {
                const usize count = _listeners.size();
                _dispatching = true;
                for (usize i = 0; i < count; ++i)
                {
                    if (i < _listeners.size() && _listeners[i].listener.stub) _listeners[i].listener(args...);
                }
                _dispatching = false;

                if (_pendingRemovals) compact();
            }

            void clear()
            {
                if (_dispatching)
                {
                    for (usize i = 0; i < _listeners.size(); ++i)
                        _listeners[i].listener.stub = nullptr;
                    _pendingRemovals = true;
                }
                else
                {
                    _listeners.clear();
                }
            }

            usize listenerCount() const { return _listeners.size(); }

        private:
            struct ListenerInfo
            {
                    ConnectionId id;
                    Delegate<T...> listener;
            };

            void compact()
            {
                for (usize i = 0; i < _listeners.size();)
                {
                    if (!_listeners[i].listener.stub)
                        _listeners.erase(i);
                    else
                        ++i;
                }
                _pendingRemovals = false;
            }

            Vector<ListenerInfo> _listeners;
            ConnectionId _nextId;
            bool _dispatching;
            bool _pendingRemovals;
    };
} // namespace seal
