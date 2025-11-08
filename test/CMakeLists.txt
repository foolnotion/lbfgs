cmake_minimum_required(VERSION 3.14)

project(lbfgsTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(lbfgs REQUIRED)
  enable_testing()
endif()

# ---- Tests ----
find_package(NANO REQUIRED COMPONENTS core function solver)

add_executable(lbfgs_test source/lbfgs_test.cpp)
target_link_libraries(lbfgs_test PRIVATE lbfgs::lbfgs NANO::core NANO::solver)
target_compile_features(lbfgs_test PRIVATE cxx_std_20)

add_test(NAME lbfgs_test COMMAND lbfgs_test)

# ---- End-of-file commands ----

add_folders(Test)
