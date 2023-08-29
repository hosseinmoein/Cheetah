add_executable(timer_tester timer_tester.cc)
target_link_libraries(timer_tester PRIVATE Threads::Threads)
target_compile_options(timer_tester
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
add_test(NAME timer_tester COMMAND timer_tester)

