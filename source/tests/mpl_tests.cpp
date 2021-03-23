// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "test_macros.hpp"

TIMEMORY_TEST_DEFAULT_MAIN

#include "timemory/timemory.hpp"

//--------------------------------------------------------------------------------------//

namespace details
{
//  Get the current tests name
inline std::string
get_test_name()
{
    return std::string(::testing::UnitTest::GetInstance()->current_test_suite()->name()) +
           "." + ::testing::UnitTest::GetInstance()->current_test_info()->name();
}

// this function consumes approximately "n" milliseconds of real time
inline void
do_sleep(long n)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(n));
}

// this function consumes an unknown number of cpu resources
inline long
fibonacci(long n)
{
    return (n < 2) ? n : (fibonacci(n - 1) + fibonacci(n - 2));
}

// this function consumes approximately "t" milliseconds of cpu time
void
consume(long n)
{
    // a mutex held by one lock
    mutex_t mutex;
    // acquire lock
    lock_t hold_lk(mutex);
    // associate but defer
    lock_t try_lk(mutex, std::defer_lock);
    // get current time
    auto now = std::chrono::steady_clock::now();
    // try until time point
    while(std::chrono::steady_clock::now() < (now + std::chrono::milliseconds(n)))
        try_lk.try_lock();
}

// get a random entry from vector
template <typename Tp>
size_t
random_entry(const std::vector<Tp>& v)
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, v.size() - 1);
    return v.at(dist(rng));
}

}  // namespace details

//--------------------------------------------------------------------------------------//

class mpl_tests : public ::testing::Test
{
protected:
    TIMEMORY_TEST_DEFAULT_SUITE_SETUP
    TIMEMORY_TEST_DEFAULT_SUITE_TEARDOWN

    TIMEMORY_TEST_DEFAULT_SETUP
    TIMEMORY_TEST_DEFAULT_TEARDOWN
};

//--------------------------------------------------------------------------------------//

template <size_t... Idx>
static constexpr auto
sequence_size(std::index_sequence<Idx...>)
{
    return sizeof...(Idx);
}

//--------------------------------------------------------------------------------------//

template <typename... Tp, size_t... Idx>
static auto
print_available(tim::type_list<Tp...>, std::index_sequence<Idx...>)
{
    std::cout << "\ntypes [t] : \n  "
              << TIMEMORY_JOIN(
                     "\n  ",
                     TIMEMORY_JOIN(" : ",
                                   TIMEMORY_JOIN(std::setw(3), std::right,
                                                 tim::component::properties<Tp>{}()),
                                   tim::demangle<Tp>())...)
              << std::endl;
    std::cout << "\ntypes [s] : \n  "
              << TIMEMORY_JOIN(
                     "\n  ",
                     TIMEMORY_JOIN(" : ",
                                   TIMEMORY_JOIN(std::setw(3), std::right,
                                                 tim::component::enumerator<Idx>{}()),
                                   tim::demangle<tim::component::enumerator_t<Idx>>())...)
              << std::endl;

    std::cout << "\nindex [t] : "
              << TIMEMORY_JOIN(", ", tim::component::properties<Tp>{}()...) << std::endl;
    std::cout << "index [s] : " << TIMEMORY_JOIN(", ", Idx...) << std::endl;
}

//--------------------------------------------------------------------------------------//

TEST_F(mpl_tests, available_index_sequence)
{
    using namespace tim;
    auto enum_sz =
        (TIMEMORY_NATIVE_COMPONENTS_END - TIMEMORY_NATIVE_COMPONENT_INTERNAL_SIZE);
    auto tuple_sz = std::tuple_size<std::tuple<TIMEMORY_COMPONENT_TYPES>>::value;

    using avail_tuple_t = stl_tuple_t<TIMEMORY_COMPONENT_TYPES>;
    using avail_tlist_t = convert_t<avail_tuple_t, tim::type_list<>>;
    using avail_idxsq_t = mpl::make_available_index_sequence<TIMEMORY_COMPONENTS_END>;
    using native_idxsq_t =
        mpl::make_available_index_sequence<TIMEMORY_NATIVE_COMPONENTS_END>;

    auto avail_comp_sz        = std::tuple_size<avail_tuple_t>::value;
    auto avail_indx_sz        = sequence_size(avail_idxsq_t{});
    auto native_avail_indx_sz = sequence_size(native_idxsq_t{});

    print_available(avail_tlist_t{}, avail_idxsq_t{});

    size_t extra = 6;  // user_{global,trace,profiler,mpip,ompt,ncclp}_bundle
#if !defined(TIMEMORY_USE_OMPT)
    extra -= 1;
#endif
#if !defined(TIMEMORY_USE_MPI) || !defined(TIMEMORY_USE_GOTCHA)
    extra -= 1;
#endif
#if !defined(TIMEMORY_USE_NCCL) || !defined(TIMEMORY_USE_GOTCHA)
    extra -= 1;
#endif

    EXPECT_EQ(enum_sz, tuple_sz);
    EXPECT_EQ(avail_comp_sz + extra, avail_indx_sz);
    EXPECT_EQ(avail_indx_sz, native_avail_indx_sz);
}

//--------------------------------------------------------------------------------------//
