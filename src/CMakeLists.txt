# packages
find_package(MPI REQUIRED)
add_definitions(-DOMPI_SKIP_MPICXX)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
message(STATUS "MPI_INCLUDE_PATH: ${MPI_INCLUDE_PATH}")

# libraries
list(APPEND field_h ../include/field/basefield.hpp ../include/field/M31.hpp)

add_library(librapp
    ${field_h}
)
set_target_properties(librapp PROPERTIES LINKER_LANGUAGE CXX)
