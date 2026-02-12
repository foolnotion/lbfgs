include(CMakeFindDependencyMacro)

# INTERFACE dependencies
find_dependency(Eigen3 CONFIG REQUIRED)
find_dependency(tl-expected CONFIG REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/lbfgsTargets.cmake")
check_required_components(lbfgs)

