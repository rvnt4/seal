#pragma once

#include "memory.h"

namespace seal
{
    template <typename T> class Vector
    {
        public:
            explicit Vector(IAllocator* alloc = nullptr) noexcept : _alloc(alloc) {}

            ~Vector() noexcept
            {
                clear();
                if (_data && _alloc) _alloc->deallocate(_data);
            }

            Vector(const Vector&) = delete;
            Vector& operator=(const Vector&) = delete;

            Vector(Vector&& other) noexcept
                : _data(other._data), _cap(other._cap), _len(other._len), _alloc(other._alloc)
            {
                other._data = nullptr;
                other._cap = 0;
                other._len = 0;
            }

            Vector& operator=(Vector&& other) noexcept
            {
                if (this != &other)
                {
                    clear();
                    if (_data && _alloc) _alloc->deallocate(_data);

                    _data = other._data;
                    _cap = other._cap;
                    _len = other._len;
                    _alloc = other._alloc;

                    other._data = nullptr;
                    other._cap = 0;
                    other._len = 0;
                }
                return *this;
            }

            bool push_back(T item) noexcept
            {
                if (_len >= _cap)
                {
                    usize new_cap = (_cap == 0) ? 4 : _cap * 2;
                    if (!reserve(new_cap)) return false;
                }

                ::new (static_cast<void*>(&_data[_len++]), seal::placement_t{}) T(static_cast<T&&>(item));
                return true;
            }

           bool reserve(usize new_cap) noexcept
            {
                if (new_cap <= _cap) return true;
                if (!_alloc) return false;

                T* new_data = static_cast<T*>(_alloc->allocate(sizeof(T) * new_cap, alignof(T)));
                if (!new_data) return false;

                for (usize i = 0; i < _len; ++i)
                {
                    ::new (static_cast<void*>(&new_data[i]), seal::placement_t{}) T(static_cast<T&&>(_data[i]));
                    _data[i].~T();
                }

                if (_data) _alloc->deallocate(_data);
                _data = new_data;
                _cap = new_cap;
                return true;
            }

            void clear() noexcept
            {
                for (usize i = 0; i < _len; ++i)
                    _data[i].~T();
                _len = 0;
            }

            bool empty() const noexcept { return _len == 0; }

            void pop_back() noexcept
            {
                if (_len > 0)
                {
                    _len--;
                    _data[_len].~T();
                }
            }

            void erase(usize idx) noexcept
            {
                if (idx >= _len) return;
                for (usize i = idx; i < _len - 1; ++i)
                {
                    _data[i] = static_cast<T&&>(_data[i + 1]);
                }
                _data[--_len].~T();
            }

            T* begin() noexcept { return _data; }
            T* end() noexcept { return _data + _len; }
            const T* begin() const noexcept { return _data; }
            const T* end() const noexcept { return _data + _len; }
            usize size() const noexcept { return _len; }

        private:
            T* _data = nullptr;
            usize _cap = 0;
            usize _len = 0;
            IAllocator* _alloc = nullptr;
    };
} // namespace seal
