/**
 * Copyright (C) 2018-2022 Daniele Salvatore Albano
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 **/

#include <cstdio>
#include <cstring>

#include <benchmark/benchmark.h>

#include "exttypes.h"
#include "spinlock.h"

#include "data_structures/hashtable/mcmp/hashtable.h"
#include "data_structures/hashtable/mcmp/hashtable_op_get.h"

#include "../tests/support.h"
#include "hashtable/mpmc/fixtures-hashtable-mpmc.h"

#include "benchmark-program.hpp"
#include "benchmark-support.hpp"

// It is possible to control the amount of threads used for the test tuning the two defines below
#define TEST_THREADS_RANGE_BEGIN (1)
#define TEST_THREADS_RANGE_END (utils_cpu_count())

static void hashtable_op_get_not_found_key(benchmark::State& state) {
    static hashtable_t* hashtable;
    hashtable_value_data_t value;

    if (state.thread_index() == 0) {
        hashtable = test_support_init_hashtable(state.range(0));
    }

    test_support_set_thread_affinity(state.thread_index());

    for (auto _ : state) {
        benchmark::DoNotOptimize(hashtable_mcmp_op_get(
                hashtable,
                test_key_1,
                test_key_1_len,
                &value));
    }

    if (state.thread_index() == 0) {
        hashtable_mcmp_free(hashtable);
    }
}

#if HASHTABLE_FLAG_ALLOW_KEY_INLINE == 1
static void hashtable_op_get_single_key_inline(benchmark::State& state) {
    static hashtable_t* hashtable;
    static hashtable_bucket_index_t bucket_index;
    static hashtable_chunk_index_t chunk_index;
    static hashtable_chunk_slot_index_t chunk_slot_index;
    hashtable_value_data_t value;
    bool result;
    char error_message[150] = {0};

    if (state.thread_index() == 0) {
        hashtable = test_support_init_hashtable(state.range(0));

        bucket_index = test_key_1_hash % hashtable->ht_current->buckets_count;
        chunk_index = HASHTABLE_TO_CHUNK_INDEX(bucket_index);
        chunk_slot_index = 0;

        HASHTABLE_SET_KEY_INLINE_BY_INDEX(
                chunk_index,
                chunk_slot_index,
                test_key_1_hash,
                test_key_1,
                test_key_1_len,
                test_value_1);
    }

    test_support_set_thread_affinity(state.thread_index());

    for (auto _ : state) {
        benchmark::DoNotOptimize((result = hashtable_mcmp_op_get(
                hashtable,
                test_key_1,
                test_key_1_len,
                &value)));

        if (!result) {
            sprintf(
                    error_message,
                    "Unable to get the key <%s> with bucket index <%lu>, chunk index <%lu> and chunk slot index <%u> for the thread <%d>",
                    test_key_1,
                    bucket_index,
                    chunk_index,
                    chunk_slot_index,
                    state.thread_index());
            state.SkipWithError(error_message);
            break;
        }
    }

    if (state.thread_index() == 0) {
        hashtable_mcmp_free(hashtable);
    }
}
#endif

static void hashtable_op_get_single_key_external(benchmark::State& state) {
    static hashtable_t* hashtable;
    static hashtable_bucket_index_t bucket_index;
    static hashtable_chunk_index_t chunk_index;
    static hashtable_chunk_slot_index_t chunk_slot_index;
    hashtable_value_data_t value;
    bool result;
    char error_message[150] = {0};

    if (state.thread_index() == 0) {
        hashtable = test_support_init_hashtable(state.range(0));

        bucket_index = test_key_1_hash % hashtable->ht_current->buckets_count;
        chunk_index = HASHTABLE_TO_CHUNK_INDEX(bucket_index);
        chunk_slot_index = 0;
        char *test_key_1_clone = (char*)ffma_mem_alloc_zero(test_key_1_len + 1);
        strncpy(test_key_1_clone, test_key_1, test_key_1_len);

        HASHTABLE_SET_KEY_EXTERNAL_BY_INDEX(
                chunk_index,
                chunk_slot_index,
                test_key_1_hash,
                test_key_1_clone,
                test_key_1_len,
                test_value_1);
    }

    test_support_set_thread_affinity(state.thread_index());

    for (auto _ : state) {
        benchmark::DoNotOptimize((result = hashtable_mcmp_op_get(
                hashtable,
                test_key_1,
                test_key_1_len,
                &value)));

        if (!result) {
            sprintf(
                    error_message,
                    "Unable to get the key <%s> with bucket index <%lu>, chunk index <%lu> and chunk slot index <%u> for the thread <%d>",
                    test_key_1,
                    bucket_index,
                    chunk_index,
                    chunk_slot_index,
                    state.thread_index());
            state.SkipWithError(error_message);
            break;
        }
    }

    if (state.thread_index() == 0) {
        hashtable_mcmp_free(hashtable);
    }
}

static void BenchArguments(benchmark::internal::Benchmark* b) {
    b
            ->Arg(256)
            ->ThreadRange(TEST_THREADS_RANGE_BEGIN, TEST_THREADS_RANGE_END)
            ->Iterations(10000000)
            ->DisplayAggregatesOnly(false);
}

BENCHMARK(hashtable_op_get_not_found_key)
        ->Apply(BenchArguments);

#if HASHTABLE_FLAG_ALLOW_KEY_INLINE == 1
BENCHMARK(hashtable_op_get_single_key_inline)
        ->Apply(BenchArguments);
#endif

BENCHMARK(hashtable_op_get_single_key_external)
        ->Apply(BenchArguments);
