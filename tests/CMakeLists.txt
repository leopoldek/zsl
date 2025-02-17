
file(GLOB test_sources CONFIGURE_DEPENDS *.cpp)
add_executable(tests ${test_sources})
target_link_libraries(tests zsl)
