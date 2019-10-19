find_path(
  GLEW_INCLUDE_DIR
  NAMES
  GL
  PATHS
  include)

find_library(
  GLEW_LIBRARY
  NAMES
  GLEW GLEWd glew32 glew32s glew32d glew32sd
  PATHS
  lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GLEW REQUIRED_VARS GLEW_LIBRARY GLEW_INCLUDE_DIR)

