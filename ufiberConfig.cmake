include(CMakeFindDependencyMacro)

find_dependency(Boost COMPONENTS system context)
find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/ufiberTargets.cmake")
