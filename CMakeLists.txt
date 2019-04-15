cmake_minimum_required(VERSION 3.8)
project(ufiber VERSION 2 LANGUAGES CXX)

option(UFIBER_SANITIZE "Build ufiber tests and examples with address & undefined sanitization enabled" OFF)
if (UFIBER_SANITIZE)
    message(STATUS "ufiber: address & undefined sanitizers enabled")
    add_compile_options(-fsanitize=address,undefined)
    link_libraries(-fsanitize=address,undefined)
endif()

find_package(Boost 1.67
             COMPONENTS
                context
                system
             REQUIRED)

find_package (Threads)


add_library(ufiber INTERFACE)
add_library(ufiber::ufiber ALIAS ufiber)

target_include_directories(ufiber INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    ufiber
    INTERFACE
        Boost::system
        Boost::context
        Threads::Threads)

target_compile_features(ufiber INTERFACE cxx_std_11)

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()


include(GNUInstallDirs)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.ipp")

install(TARGETS ufiber
        EXPORT ufiberTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(UFIBER_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
unset(CMAKE_SIZEOF_VOID_P)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "ufiberConfigVersion.cmake"
    COMPATIBILITY AnyNewerVersion)

set(CMAKE_SIZEOF_VOID_P ${UFIBER_SIZEOF_VOID_P})
unset(UFIBER_SIZEOF_VOID_P)

install(FILES
            "ufiberConfig.cmake"
            "${CMAKE_BINARY_DIR}/ufiberConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ufiber)

install(EXPORT ufiberTargets
        FILE ufiberTargets.cmake
        NAMESPACE ufiber::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ufiber)
