set (ufiber_tests_srcs
    ufiber/spawn.cpp
    ufiber/spawn_discard.cpp
    ufiber/yield_token_conversion.cpp)

function (ufiber_add_test test_file)
    get_filename_component(target_name ${test_file} NAME_WE)
    add_executable(${target_name} ${test_file})
    target_link_libraries(${target_name} ufiber::ufiber)

    add_test(NAME "${target_name}_tests"
             COMMAND ${target_name})
endfunction(ufiber_add_test)

foreach(test_src_name IN ITEMS ${ufiber_tests_srcs})
    ufiber_add_test(${test_src_name})
endforeach()
