#pragma once

#include <cassert>
#include <cstddef>
#include <queue>
#include <vector>

namespace coro::detail
{

/**
 * @brief marked_buffer is a fixed buffer which can lease buffer item to outside
 *
 * @tparam type: buffer item type
 * @tparam length: length of fixed buffer
 */
template<typename Type, size_t length>
struct marked_buffer
{
    struct item
    {
        inline auto valid() -> bool { return idx >= 0; }
        inline auto set_invalid() -> void
        {
            idx = -1;
            ptr = nullptr;
        }
        int   idx; // idx < 0 means item invalid
        Type* ptr;
    };

    marked_buffer() noexcept { init(); }

    void init() noexcept
    {
        std::queue<int> temp;
        que.swap(temp);
    }

    void set_data(const std::vector<Type>& values)
    {
        assert(data.size() == length && "");

        for (int i = 0; i < length; i++)
        {
            que.push(i);
        }

        for (int i = 0; i < length; i++)
        {
            data[i] = values[i];
        }
    }

    // Allocate a buffer item to outsize
    item borrow() noexcept
    {
        if (que.empty())
        {
            return item{.idx = -1, .ptr = nullptr};
        }
        auto idx = que.front();
        que.pop();
        return item{.idx = idx, .ptr = &(data[idx])};
    }

    // Recycle a buffer item from outsize
    void return_back(item it) noexcept
    {
        // ignore invalid item
        if (!it.valid())
        {
            return;
        }
        it.ptr = nullptr;
        que.push(it.idx);
    }

    Type data[length];
    // Every index in que means this index is free now
    std::queue<int> que;
};

}; // namespace coro::detail