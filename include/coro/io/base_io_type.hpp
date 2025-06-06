#include "coro/engine.hpp"

namespace coro::io::detail
{

// If you need to use the feature "IOSQE_FIXED_FILE", just include fixed_fds as member varaible,
// this class will automatically fetch fixed fds from local engine's io_uring
struct fixed_fds
{
    fixed_fds() noexcept
    {
        // borrow fixed fd from uring
        item = ::coro::detail::local_engine().get_uring().get_fixed_fd();
    }

    ~fixed_fds() noexcept { return_back(); }

    inline auto assign(int& fd, int& flag) noexcept -> void
    {
        if (!item.valid())
        {
            return;
        }
        *(item.ptr) = fd;
        fd          = item.idx;
        flag |= IOSQE_FIXED_FILE;
        ::coro::detail::local_engine().get_uring().update_register_fixed_fds(item.idx);
    }

    inline auto return_back() noexcept -> void
    {
        // return back fixed fd to uring
        if (item.valid())
        {
            ::coro::detail::local_engine().get_uring().back_fixed_fd(item);
            item.set_invalid();
        }
    }

    ::coro::uring::uring_fds_item item;
};
}; // namespace coro::io::detail
