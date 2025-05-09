/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_iostream.hpp>
#include <mlplc/mlplc.hpp>

#include <zephyr/sys/sys_heap.h>

#include <stdio.h>
#include <functional>
#include <cstdint>
#include <string_view>

using namespace mlplc;
using magic_enum::iostream_operators::operator<<;
using namespace std::chrono_literals;

constexpr std::size_t THREAD_STRESS_TEST_COUNT = 20;
constexpr std::size_t THREAD_STRESS_TEST_STACK_SIZE = 1024;

constexpr std::size_t MY_STATIC_THREAD_STACK_SIZE = 4500;
constexpr std::string_view BUTTON = "BUTTON";
constexpr std::string_view LED_R = "LED_R";
constexpr std::string_view LED_G = "LED_G";
constexpr std::string_view LED_B = "LED_B";

LOG_MODULE_REGISTER(app);

K_THREAD_STACK_DEFINE(my_static_thread_stack, MY_STATIC_THREAD_STACK_SIZE);

void on_fatal_error(std::string const& thread_name, FatalReason reason) {
    LOG_ERR("on_fatal_error: thread=%s reason=%s", thread_name.c_str(), fatal_reason_to_str(reason));
}

void my_static_thread_func(void) {
    LOG_INF("my_static_thread_func:");
}

void my_thread_void_func(void) {
    LOG_INF("my_thread_void:");
}

void my_thread_func(int const arg) {
    periph::DigitalInput const input0(0);
    LOG_INF("my_thread_func: arg=%i input0.level=%s", arg, level_to_str(input0.level()));
}

void led_handle(std::chrono::milliseconds duration) {
    periph::DigitalInput button(BUTTON);
    periph::DigitalOutput led_r(LED_R);
    auto const t_stop = sys::uptime() + duration;
    while (sys::uptime() < t_stop) {
        led_r.set_level(!button.level());
        sys::sleep(1ms);
    }
}

void thread_stress_test_func(int arg) {
    LOG_INF("thread_stress_test_func: arg=%i", arg);
    sys::sleep(25ms);
}

void stack_overflowed_thread_func() {
    volatile uint8_t dummy[1024];
    for (auto& d : dummy) {
        d = 123;
    }
    LOG_INF("stack_overflowed_thread_func: %u", dummy[sizeof(dummy)-1]);
}

void exception_thread_func() {
    LOG_INF("exception_thread_func:");
    throw std::runtime_error("exception_thread_func:");
}

int main(void) {
    mlplc::init(on_fatal_error);
    LOG_INF("Hello from MLPLC!");

    sys::mem_usage();

    LOG_INF("Exception test..");
    try {
        throw std::runtime_error("Example exception");
    } catch (std::exception const& ex) {
        LOG_INF("Exception caught: %s", ex.what());
    }

    try {
        {
            LOG_INF("Try create static thread..");
            auto my_static_thread = sys::Thread<>("my_static_thread", my_static_thread_func,
                my_static_thread_stack, MY_STATIC_THREAD_STACK_SIZE, 0);
            my_static_thread.start();
            LOG_INF("My static thread created! Join..");
            my_static_thread.join();
            LOG_INF("Join complete, is_finished=%i", my_static_thread.is_finished());
        }
        sys::sleep(10ms);
        {
            LOG_INF("Try create dyn void thread..");
            auto my_void_thread = sys::Thread<>("my_thread_void", my_thread_void_func, 1024, 0);
            LOG_INF("My void thread created, is_finished=%i, Join..", my_void_thread.is_finished());
            my_void_thread.start();
            my_void_thread.join();
            LOG_INF("Join complete, is_finished=%i", my_void_thread.is_finished());
        }
        sys::sleep(10ms);
        {
            LOG_INF("Try create dyn thread with args..");
            auto my_thread = sys::Thread<int>("my_dyn_thread", my_thread_func, 1024, 0, 123);
            my_thread.start();
            LOG_INF("My thread created! Join..");
            my_thread.join();
        }

        {
            sys::sleep(10ms);
            LOG_INF("Try create dyn thread for led handle..");
            auto led_thread = sys::Thread<std::chrono::milliseconds>("led_handle", led_handle, 1024, 0, 10000ms);
            led_thread.start();
            sys::sleep(10ms);
            LOG_INF("led_thread stack: used=%zu total=%zu", led_thread.stack_usage(), led_thread.stack_size());
            led_thread.join();
            LOG_INF("led_thread aborted, is_finished=%i", led_thread.is_finished());
        }

        {
            std::vector<std::shared_ptr<sys::Thread<int>>> threads{};
            for (std::size_t i = 0; i < THREAD_STRESS_TEST_COUNT; i++) {
                LOG_INF("Create stress thread %zu..", i);
                std::string name = "thread_stress_test" + std::to_string(i);
                auto thread = std::make_shared<sys::Thread<int>>(name, thread_stress_test_func,
                    THREAD_STRESS_TEST_STACK_SIZE, 0, i);
                thread->start();
                threads.push_back(thread);
            }
            for (auto& th : threads) {
                th->join();
            }
        }

        {
            // sys::sleep(10ms);
            // LOG_INF("Try create dyn stack overflowed thread..");
            // auto stack_overflowed_thread = sys::Thread<>("stack_overflowed_thread", stack_overflowed_thread_func, 500, 0);
            // stack_overflowed_thread.start();
            // LOG_INF("Stack overflowed thread created!");
        }

        {
            sys::sleep(10ms);
            LOG_INF("Try create dyn exception thread..");
            auto exception_thread = sys::Thread<>("exception_thread", exception_thread_func, 2048, 0);
            exception_thread.start();
            LOG_INF("exception_thread created!");
            exception_thread.join();
            LOG_INF("exception_thread.error=%s", exception_thread.error()->c_str());
        }

        {
            periph::DigitalOutput led_r("LED_R");
            periph::DigitalOutput led_g("LED_G");
            periph::DigitalOutput led_b("LED_B");
            led_r.pulse_start(500ms, 500ms);
            led_g.pulse_start(400ms, 400ms);
            led_b.pulse_start(300ms, 300ms);
            sys::sleep(10000ms);
        }
    } catch (std::exception const& ex) {
        LOG_INF("Example failed: %s", ex.what());
    } catch (...) {
        LOG_INF("Example failed: Unknown exception.");
    }

    sys::mem_usage();
    sys::reset();

    return 0;
}
