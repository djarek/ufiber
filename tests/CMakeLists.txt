set (ufiber_tests_srcs
    ufiber/spawn_discard.cpp
    ufiber/spawn.cpp
    ufiber/yield_token_conversion.cpp)

function (ufiber_add_test test_file)
    get_filename_component(target_name ${test_file} NAME_WE)
    add_executable(${target_name} ${test_file})
    target_link_libraries(${target_name} ufiber Boost::system)
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -pedantic -std=c++11)
    target_compile_definitions(${target_name} PRIVATE)

    add_test(NAME "${target_name}_tests"
             COMMAND ${target_name})
endfunction(ufiber_add_test)

foreach(test_src_name IN ITEMS ${ufiber_tests_srcs})
    ufiber_add_test(${test_src_name})
endforeach()
