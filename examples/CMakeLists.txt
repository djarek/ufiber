#find_package(ufiber REQUIRED)

add_executable(echo_example echo.cpp)
target_link_libraries(echo_example PRIVATE
    ufiber::ufiber
    Boost::system)
