// The first "test" is of course whether the header compiles standalone
#include "pplx/pplx_utils.h"

#include "bst/test/test.h"

namespace
{
    template <typename ReturnType>
    inline pplx::task<ReturnType> task_from_default() { return pplx::task_from_result(ReturnType()); }

    template <>
    inline pplx::task<void> task_from_default() { return pplx::task_from_result(); }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEMPLATE_TEST_CASE_2(testPplxWhenAll, ReturnType, void, int)
{
    pplx::task<ReturnType> default_task;
    pplx::task<ReturnType> successful_task1 = task_from_default<ReturnType>();
    pplx::task<ReturnType> successful_task2 = task_from_default<ReturnType>();
    pplx::task<ReturnType> failed_task = pplx::task_from_exception<ReturnType>(std::runtime_error("failed"));
    pplx::task_completion_event<ReturnType> tce;
    pplx::task<ReturnType> incomplete_task(tce);
    using final_task = decltype(pplx::ranges::when_all(std::declval<std::initializer_list<pplx::task<ReturnType>>>()));

    BST_REQUIRE_THROW(failed_task.wait(), std::runtime_error);
    BST_REQUIRE_THROW(default_task.wait(), pplx::invalid_operation);

    {
        auto tasks = { successful_task1, successful_task2 };
        bool continuation = false;
        pplx::ranges::when_all(tasks).then([&](final_task finally)
        {
            finally.wait();
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }

    {
        auto tasks = { successful_task1, failed_task, incomplete_task };
        bool continuation = false;
        pplx::ranges::when_all(tasks).then([&](final_task finally)
        {
            BST_REQUIRE_THROW(finally.wait(), std::runtime_error);
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }

    {
        auto tasks = { default_task, incomplete_task };
        bool continuation = false;
        pplx::ranges::when_all(tasks).then([&](final_task finally)
        {
            BST_REQUIRE_THROW(finally.wait(), pplx::invalid_operation);
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEMPLATE_TEST_CASE_2(testPplxWhenAny, ReturnType, void, int)
{
    pplx::task<ReturnType> default_task;
    pplx::task<ReturnType> successful_task1 = task_from_default<ReturnType>();
    pplx::task<ReturnType> successful_task2 = task_from_default<ReturnType>();
    pplx::task<ReturnType> failed_task = pplx::task_from_exception<ReturnType>(std::runtime_error("failed"));
    pplx::task_completion_event<ReturnType> tce;
    pplx::task<ReturnType> incomplete_task(tce);
    using final_task = decltype(pplx::ranges::when_any(std::declval<std::initializer_list<pplx::task<ReturnType>>>()));

    BST_REQUIRE_THROW(failed_task.wait(), std::runtime_error);
    BST_REQUIRE_THROW(default_task.wait(), pplx::invalid_operation);

    {
        auto tasks = { successful_task1, successful_task2 };
        bool continuation = false;
        pplx::ranges::when_any(tasks).then([&](final_task finally)
        {
            finally.wait();
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }

    {
        auto tasks = { successful_task1, failed_task, incomplete_task };
        bool continuation = false;
        pplx::ranges::when_any(tasks).then([&](final_task finally)
        {
            finally.wait();
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }

    {
        auto tasks = { default_task };
        bool continuation = false;
        pplx::ranges::when_any(tasks).then([&](final_task finally)
        {
            BST_REQUIRE_THROW(finally.wait(), pplx::invalid_operation);
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }
}
