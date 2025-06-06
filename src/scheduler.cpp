#include "coro/scheduler.hpp"
#include "coro/meta_info.hpp"

namespace coro
{
auto scheduler::init_impl(size_t ctx_cnt) noexcept -> void
{
    detail::init_meta_info();

    m_ctx_cnt = ctx_cnt;
    m_ctxs    = detail::ctx_container{};
    m_ctxs.reserve(m_ctx_cnt);
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs.emplace_back(std::make_unique<context>());
    }
    m_dispatcher.init(m_ctx_cnt, &m_ctxs);
    m_ctx_stop_flag = stop_flag_type(m_ctx_cnt, detail::atomic_ref_wrapper<int>{.val = 1});
    m_stop_token    = m_ctx_cnt;

#ifdef ENABLE_MEMORY_ALLOC
    coro::allocator::memory::mem_alloc_config config;
    m_mem_alloc.init(config);
    ginfo.mem_alloc = &m_mem_alloc;
#endif
}

auto scheduler::start_impl() noexcept -> void
{
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->set_stop_cb(
            [&, i]()
            {
                auto cnt = std::atomic_ref(this->m_ctx_stop_flag[i].val).fetch_and(0, memory_order_acq_rel);
                if (this->m_stop_token.fetch_sub(cnt) == cnt)
                {
                    // Stop token decrease to zero, it's time to close all context
                    this->stop_impl();
                }
            });
        m_ctxs[i]->start();
    }
}

auto scheduler::loop_impl() noexcept -> void
{
    start_impl();
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->join();
    }
}

auto scheduler::stop_impl() noexcept -> void
{
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->notify_stop();
    }
}

auto scheduler::submit_task_impl(std::coroutine_handle<> handle) noexcept -> void
{
    assert(this->m_stop_token.load(std::memory_order_acquire) != 0 && "error! submit task after scheduler loop finish");
    size_t ctx_id = m_dispatcher.dispatch();
    m_stop_token.fetch_add(
        1 - std::atomic_ref(m_ctx_stop_flag[ctx_id].val).fetch_or(1, memory_order_acq_rel), memory_order_acq_rel);
    m_ctxs[ctx_id]->submit_task(handle);
}
}; // namespace coro
