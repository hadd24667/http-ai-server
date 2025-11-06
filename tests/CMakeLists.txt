add_executable(run_tests test_scheduler.cpp test_threadpool.cpp)
add_test(NAME run_tests COMMAND run_tests)
