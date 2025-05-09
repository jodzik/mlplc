#pragma once

#include "macro.hpp"
#include <mlplc/exception.hpp>
#include <mlplc/sys/mutex.hpp>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <mutex>
#include <cstdint>
#include <functional>
#include <memory>
#include <expected>
#include <optional>
#include <tuple>

namespace mlplc {
namespace sys {

namespace {

}

namespace _private {

}

template<typename... Args> static void thread_entry(void* p1, void* p2, void* p3);

template<typename... Args>
class Thread {
private:
    static constexpr uint8_t THREAD_STATE_DEAD = 0x08;

public:
    Thread(
        std::string_view name,
        std::function<void(Args...)> func,
        k_thread_stack_t* stack,
        std::size_t stack_size,
        uint8_t priority,
        Args&&... args) :
        _name(name),
        _func(func),
        _args(args...),
        _stack_size(stack_size),
        _priority(priority)
    {
        this->_thread = std::shared_ptr<struct k_thread>((struct k_thread*)k_object_alloc(K_OBJ_THREAD),
            [](struct k_thread* th){k_object_free(th);});
        ASSERT(this->_thread, ExceptionType::NoMemory, "Fail to allocate thread object");

        k_tid_t id = k_thread_create(this->_thread.get(), stack, this->_stack_size,
            thread_entry<Args...>, this, NULL, NULL, this->_priority, 0, K_FOREVER);
        ASSERT(id, ExceptionType::Unknown, "Fail to create thread");

        std::string name_c(name);
        CCALL(k_thread_name_set(this->_thread.get(), name_c.c_str()));
    }

    Thread(
        std::string_view name,
        std::function<void(Args...)> func,
        std::size_t stack_size,
        uint8_t priority,
        Args&&... args) :
        _name(name),
        _func(func),
        _args(args...),
        _stack_size(stack_size),
        _priority(priority)
    {
        this->_stack = std::shared_ptr<k_thread_stack_t>(k_thread_stack_alloc(this->_stack_size, 0),
            [](k_thread_stack_t* stack){k_thread_stack_free(stack);});
        ASSERT(this->_stack, ExceptionType::NoMemory);

        this->_thread = std::shared_ptr<struct k_thread>((struct k_thread*)k_object_alloc(K_OBJ_THREAD),
            [](struct k_thread* th){printk("Destroy thread object..\n"); k_object_free(th);});
        ASSERT(this->_thread, ExceptionType::NoMemory, "Fail to allocate thread object");

        k_tid_t id = k_thread_create(this->_thread.get(), this->_stack.get(), this->_stack_size,
            thread_entry<Args...>, this, NULL, NULL, this->_priority, 0, K_FOREVER);
        ASSERT(id, ExceptionType::Unknown, "Fail to create thread");

        std::string name_c(name);
        CCALL(k_thread_name_set(this->_thread.get(), name_c.c_str()));
    }

    ~Thread() {
        if (this->_thread) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wterminate"
            ASSERT(this->is_finished(), ExceptionType::DestroyingRunningThread, this->_name);
#pragma GCC diagnostic pop
        }
    }

    void start() {
        std::lock_guard<Mutex> lock(this->_mutex);
        k_thread_start(this->_thread.get());
    }

    void join() {
        CCALL(k_thread_join(this->_thread.get(), K_FOREVER));
    }

    /** Abort thread execution.
     * @warning Be careful, this function hard deletes the thread and thread stack,
     *          and objects currently allocated on stack are NOT destroyed in the correct way -
     *          destructors do not will be called. Use this if thread not contain complex class object on stack.
     *          For correct terminate thread you may manually cause exception in thread. 
     */
    void abort() {
        std::lock_guard<Mutex> lock(this->_mutex);
        k_thread_abort(this->_thread.get());
    }

    bool is_finished() const {
        std::lock_guard<Mutex> lock(this->_mutex);
        return this->_thread->base.thread_state & THREAD_STATE_DEAD;
    }

    std::optional<std::string> error() {
        std::lock_guard<Mutex> lock(this->_mutex);
        return this->_error;
    }

    std::size_t stack_usage() const {
        std::lock_guard<Mutex> lock(this->_mutex);
        std::size_t usage = 0;
        CCALL(k_thread_stack_space_get(this->_thread.get(), &usage));
        return usage;
    }

    std::size_t stack_size() const {
        return this->_stack_size;
    }

    std::string const& name() const {
        return this->_name;
    }

    Thread(Thread&) = delete;
    Thread(Thread const&) = delete;
    Thread(Thread&&) = delete;

private:
    mutable Mutex _mutex;

    std::string _name;
    std::function<void(Args...)> const _func;
    std::tuple<Args...> const _args;
    std::size_t const _stack_size;
    uint8_t _priority;

    std::shared_ptr<struct k_thread> _thread = nullptr;
    std::shared_ptr<k_thread_stack_t> _stack = nullptr;
    std::optional<std::string> _error = std::nullopt;

    friend void thread_entry<Args...>(void*, void*, void*);
};

template<typename... Args>
static void thread_entry(void* p1, void* p2, void* p3) {
    Thread<Args...>* _this = reinterpret_cast<Thread<Args...>*>(p1);
    try {
        _this->_func(std::get<Args>(_this->_args)...);
    } catch (std::exception const& ex) {
        _this->_error = std::string(ex.what());
    }
}

} // namespace sys
} // namespace mlplc