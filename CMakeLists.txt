cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    lbfgs
    VERSION 0.1.0
    DESCRIPTION "Modern C++20 implementation of the Limited-memory BFGS optimization algorithm"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES NONE
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(lbfgs_lbfgs INTERFACE)
add_library(lbfgs::lbfgs ALIAS lbfgs_lbfgs)

set_property(
    TARGET lbfgs_lbfgs PROPERTY
    EXPORT_NAME lbfgs
)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(tl-expected REQUIRED)

target_include_directories(
    lbfgs_lbfgs ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(lbfgs_lbfgs INTERFACE ${incdir})

target_link_libraries(lbfgs_lbfgs INTERFACE Eigen3::Eigen tl::expected)

target_compile_features(lbfgs_lbfgs INTERFACE cxx_std_20)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${lbfgs_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT lbfgs_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of lbfgs"
  )
endif()

include(cmake/dev-mode.cmake)
