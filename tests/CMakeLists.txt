enable_testing()

# Test scheduler
add_executable(test_scheduler test_scheduler.cpp)
target_link_libraries(test_scheduler pthread)

add_test(NAME test_scheduler COMMAND test_scheduler)

# Test threadpool
add_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool pthread)

add_test(NAME test_threadpool COMMAND test_threadpool)
