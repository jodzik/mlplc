/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mlplc/mlplc.hpp>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/printk.h>

#include <stdio.h>
#include <functional>
#include <cstdint>
#include <string_view>

constexpr std::size_t STATIC_THREAD_STACK_SIZE = 1024;

LOG_MODULE_REGISTER(app);

K_THREAD_STACK_DEFINE(my_static_thread_stack, STATIC_THREAD_STACK_SIZE);

void my_static_thread_func(void* arg1, void* arg2, void* arg3) {
    mlplc::periph::DigitalInput input0("BUTTON");
    LOG_INF("my_static_thread_func: input0=%s", mlplc::level_to_str(input0.level()));
}

void my_dyn_thread_func(void* arg1, void* arg2, void* arg3) {
    mlplc::periph::DigitalInput input0("BUTTON");
    LOG_INF("my_dyn_thread_func: input0=%s", mlplc::level_to_str(input0.level()));
}

int test_static_thread(k_thread_stack_t* stack, size_t stack_size) {
    struct k_thread* thread = (struct k_thread*)k_object_alloc(K_OBJ_THREAD);
    if (!thread) {
        LOG_ERR("Fail to alloc thread handle.");
        return -1;
    }

    k_tid_t id = k_thread_create(thread, stack, stack_size,
        my_static_thread_func, NULL, NULL, NULL, 0, 0, K_NO_WAIT);

    if (!id) {
        LOG_ERR("Fail to create thread.");
        return -1;
    }
    k_thread_name_set(id, "my_static_thread");

    LOG_INF("my_static_thread created. Join..");
    int rc = k_thread_join(id, K_FOREVER);
    if (0 != rc) {
        LOG_ERR("Fail to join.");
        return rc;
    }
    k_object_free(thread);

    return 0;
}

int test_dyn_thread(size_t stack_size) {
    struct k_thread* thread = (struct k_thread*)k_object_alloc(K_OBJ_THREAD);
    if (!thread) {
        LOG_ERR("Fail to alloc thread handle.");
        return -1;
    }

    k_thread_stack_t* stack = k_thread_stack_alloc(stack_size, 0);
    if (!stack) {
        LOG_ERR("Fail to alloc thread stack.");
    }

    k_tid_t id = k_thread_create(thread, stack, stack_size,
        my_dyn_thread_func, NULL, NULL, NULL, 0, 0, K_NO_WAIT);

    if (!id) {
        LOG_ERR("Fail to create thread.");
        return -1;
    }
    k_thread_name_set(id, "my_dyn_thread");

    LOG_INF("my_dyn_thread created. Join..");
    int rc = k_thread_join(id, K_FOREVER);
    if (0 != rc) {
        LOG_ERR("Fail to join.");
        return rc;
    }
    k_object_free(thread);
    rc = k_thread_stack_free(stack);
    if (0 != rc) {
        LOG_ERR("Fail to stack free.");
        return rc;
    }

    return 0;
}

int main(void) {
    // mlplc::init();
    LOG_INF("main:");

    test_static_thread(my_static_thread_stack, STATIC_THREAD_STACK_SIZE);
    test_dyn_thread(1024);
    test_dyn_thread(1024);

    return 0;
}
