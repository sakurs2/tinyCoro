#include "coro/engine.hpp"
#include "coro/log.hpp"
#include "coro/net/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    linfo.egn            = this;
    m_num_io_wait_submit = 0;
    m_num_io_running     = 0;
    m_upxy.init(config::kEntryLength);
}

auto engine::deinit() noexcept -> void
{
    m_upxy.deinit();
    m_num_io_wait_submit = 0;
    m_num_io_running     = 0;
    if (!m_task_queue.was_empty())
    {
        log::warn("task queue isn't empty when engine deinit");
    }
    mpmc_queue<coroutine_handle<>> task_queue;
    m_task_queue.swap(task_queue);
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    auto coro = m_task_queue.pop();
    assert(bool(coro));
    return coro;
}

// TODO: This function is very important to tinyCoro normally run,
// after every update for tinyCoro, this function should be carefully checked!
auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // FIXME: This way will cause stack overflow
    assert(handle != nullptr && "engine get nullptr task handle");
    if (m_task_queue.try_push(handle))
    {
        wake_up();
    }
    else if (is_in_working_state())
    {
        // A forced push will cause thread blocked forever,
        // so exec this task directly
        exec_task(handle);

        // if (is_local_engine())
        // {
        //     // Engine push task to itself, it will be blocked forever,
        //     // so exec this task first, which make engine won't block itself
        //     exec_task(handle);
        // }
        // else
        // {
        //     // Other thread push task to engine, because engine won't block itself,
        //     // so just block until engine's task queue has free position
        //     // This case can also call exec_task(handle) directly
        //     m_task_queue.push(handle);
        // }
    }
    else
    {
        log::error("push task out of capacity before work thread run!");
    }
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    exec_task(coro);
}

auto engine::exec_task(coroutine_handle<> handle) -> void
{
    handle.resume();
    if (handle.done())
    {
        clean(handle);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<net::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::do_io_submit() noexcept -> void
{
    // int num_task_wait = m_num_io_wait_submit.load(std::memory_order_acquire);
    if (m_num_io_wait_submit > 0)
    {
        // A submit call will submit all io waited.
        // If polling mode is enabled, call submit may do nothing,
        // but still need to call submit to wake up kernel thread
        [[CORO_MAYBE_UNUSED]] auto _ = m_upxy.submit();
        // assert(num_task_wait == 0);
        m_num_io_running += m_num_io_wait_submit;
        m_num_io_wait_submit = 0;
    }
}

auto engine::poll_submit() noexcept -> void
{
    do_io_submit();

    // TODO: reduce call wait
    auto cnt = m_upxy.wait_eventfd();
    if (!wake_by_cqe(cnt))
    {
        return;
    }

    auto num = m_upxy.peek_batch_cqe(m_urc.data(), m_num_io_running);

    if (num != 0)
    {
        for (int i = 0; i < num; i++)
        {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(num);
        m_num_io_running -= num;
    }
}

auto engine::wake_up(uint64_t val) noexcept -> void
{
    m_upxy.write_eventfd(val);
}
}; // namespace coro::detail
