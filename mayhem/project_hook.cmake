# Injected with -DCMAKE_PROJECT_Cockatrice_INCLUDE=<this file> by mayhem/build.sh (no upstream file is
# edited). It defers mayhem/fuzzers.cmake to the end of the top-level CMakeLists.txt, when every
# upstream library target exists, so the harnesses link the real (sanitized) libraries with their
# usage requirements (include dirs, Qt, protobuf).
get_filename_component(COCKATRICE_FUZZERS_CMAKE "${CMAKE_CURRENT_LIST_DIR}/fuzzers.cmake" ABSOLUTE)
cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY \"${CMAKE_SOURCE_DIR}\" CALL include \"${COCKATRICE_FUZZERS_CMAKE}\")")
