// The first "test" is of course whether the header compiles standalone
#include "pplx/pplx_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testWhenAll)
{
    pplx::task<void> default_task;
    pplx::task<void> successful_task1 = pplx::task_from_result();
    pplx::task<void> successful_task2 = pplx::task_from_result();
    pplx::task<void> failed_task = pplx::task_from_exception<void>(std::runtime_error("failed"));

    BST_REQUIRE_THROW(failed_task.wait(), std::runtime_error);

    {
        auto tasks = { successful_task1, successful_task2 };
        bool continuation = false;
        pplx::ranges::when_all(tasks).then([&](pplx::task<void> finally)
        {
            finally.get();
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }

    {
        auto tasks = { successful_task1, failed_task };
        bool continuation = false;
        pplx::ranges::when_all(tasks).then([&](pplx::task<void> finally)
        {
            BST_REQUIRE_THROW(finally.get(), std::runtime_error);
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }

    BST_REQUIRE_THROW(default_task.wait(), pplx::invalid_operation);

    {
        auto tasks = { successful_task1, default_task };
        bool continuation = false;
        pplx::ranges::when_all(tasks).then([&](pplx::task<void> finally)
        {
            BST_REQUIRE_THROW(finally.get(), pplx::invalid_operation);
            continuation = true;
        }).wait();
        BST_REQUIRE(continuation);
    }
}
