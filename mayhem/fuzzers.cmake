# Fuzz harness targets (see project_hook.cmake). FUZZ_ENGINE: link flag of the fuzzing engine;
# FUZZ_STANDALONE_MAIN: LLVM's run-once driver; FUZZ_LSAN_OFF: mayhem/lsan_off.cc.
set(_h "${CMAKE_SOURCE_DIR}/mayhem/harnesses")
set(_out "${CMAKE_BINARY_DIR}/fuzz")

function(cockatrice_fuzzer name)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;INCLUDES" ${ARGN})
  foreach(kind fuzzer standalone)
    if(kind STREQUAL "fuzzer")
      set(tgt ${name})
      set(extra "")
    else()
      set(tgt ${name}-standalone)
      set(extra "${FUZZ_STANDALONE_MAIN}")
    endif()
    add_executable(${tgt} ${_h}/${name}.cpp ${ARG_SOURCES} ${FUZZ_LSAN_OFF} ${extra})
    target_link_libraries(${tgt} PRIVATE ${ARG_LIBS})
    target_include_directories(${tgt} PRIVATE ${_h} ${ARG_INCLUDES})
    set_target_properties(${tgt} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_out} AUTOMOC OFF)
    if(kind STREQUAL "fuzzer")
      target_link_options(${tgt} PRIVATE ${FUZZ_ENGINE})
    endif()
  endforeach()
endfunction()

cockatrice_fuzzer(fuzz_deck_cod LIBS libcockatrice_deck_list Qt6::Core)
cockatrice_fuzzer(fuzz_deck_plain LIBS libcockatrice_deck_list Qt6::Core)
cockatrice_fuzzer(fuzz_card_xml SOURCES ${VERSION_STRING_CPP} LIBS libcockatrice_card libcockatrice_interfaces Qt6::Core)
cockatrice_fuzzer(fuzz_expression LIBS libcockatrice_utility Qt6::Core)
cockatrice_fuzzer(
  fuzz_forwarded_address
  SOURCES ${CMAKE_SOURCE_DIR}/servatrice/src/forwarded_address.cpp ${CMAKE_SOURCE_DIR}/servatrice/src/trusted_sources.cpp
  INCLUDES ${CMAKE_SOURCE_DIR}/servatrice/src
  LIBS Qt6::Core Qt6::Network
)
cockatrice_fuzzer(fuzz_local_server SOURCES ${VERSION_STRING_CPP} LIBS libcockatrice_network libcockatrice_rng libcockatrice_protocol Qt6::Core)

# Client spoiler download: SpoilerFile::validate (cockatrice/src) + the card XML parsers.
cockatrice_fuzzer(
  fuzz_spoiler_xml
  SOURCES ${CMAKE_SOURCE_DIR}/cockatrice/src/client/network/update/card_spoiler/spoiler_file.cpp ${VERSION_STRING_CPP}
  INCLUDES ${CMAKE_SOURCE_DIR}/cockatrice/src
  LIBS libcockatrice_card libcockatrice_interfaces Qt6::Core
)

# Oracle (not configured in this server-only build): compile the sources the importer, tag database
# and OSDaB UnZip need straight into the harnesses, as upstream's tests/oracle does. zlib enables the
# zip/gzip paths (HAS_ZLIB); liblzma is not installed, so xz stays disabled as in a zlib-only Oracle.
find_package(ZLIB REQUIRED)
set(_oracle ${CMAKE_SOURCE_DIR}/oracle/src)
set(_oracle_zip ${_oracle}/zip/unzip.cpp ${_oracle}/zip/zipglobal.cpp)
cockatrice_fuzzer(
  fuzz_oracle_json
  SOURCES ${VERSION_STRING_CPP} ${_oracle}/oracleimporter.cpp ${_oracle}/parsehelpers.cpp ${_oracle}/raw_json_scanner.cpp
          ${_oracle}/tag_database.cpp ${_oracle_zip}
  INCLUDES ${_oracle}
  LIBS libcockatrice_card libcockatrice_interfaces Qt6::Core ZLIB::ZLIB
)
cockatrice_fuzzer(fuzz_oracle_zip SOURCES ${_oracle_zip} INCLUDES ${_oracle} LIBS Qt6::Core ZLIB::ZLIB)
cockatrice_fuzzer(
  fuzz_tag_database
  SOURCES ${_oracle}/tag_database.cpp ${_oracle_zip}
  INCLUDES ${_oracle}
  LIBS Qt6::Core ZLIB::ZLIB
)
foreach(_t fuzz_oracle_json fuzz_oracle_zip fuzz_tag_database)
  foreach(_k "" "-standalone")
    target_compile_definitions(${_t}${_k} PRIVATE HAS_ZLIB)
    set_target_properties(${_t}${_k} PROPERTIES AUTOMOC ON)
  endforeach()
endforeach()

# Client replays: libcockatrice_game and the libraries it links (settings, filters) are only configured
# WITH_CLIENT/WITH_ORACLE upstream, and add_subdirectory is not allowed in this deferred hook, so they
# are defined here under upstream's target names, from exactly the .cpp files upstream's CMakeLists list
# (every .cpp in each library directory), with the same include dirs and link libraries.
find_package(Qt6 REQUIRED COMPONENTS Gui Concurrent)
function(cockatrice_fuzz_lib name)
  cmake_parse_arguments(ARG "" "" "LIBS;INCLUDES" ${ARGN})
  file(GLOB_RECURSE _srcs ${CMAKE_SOURCE_DIR}/${name}/libcockatrice/*.cpp ${CMAKE_SOURCE_DIR}/${name}/libcockatrice/*.h)
  add_library(${name} STATIC ${_srcs})
  set_target_properties(${name} PROPERTIES AUTOMOC ON)
  target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/${name} ${ARG_INCLUDES})
  target_link_libraries(${name} PUBLIC ${ARG_LIBS})
endfunction()
if(NOT TARGET libcockatrice_game)
  cockatrice_fuzz_lib(libcockatrice_settings LIBS libcockatrice_card libcockatrice_utility Qt6::Core
                      INCLUDES ${CMAKE_SOURCE_DIR}/cockatrice/src/client/network)
  cockatrice_fuzz_lib(libcockatrice_filters LIBS libcockatrice_card Qt6::Core)
  cockatrice_fuzz_lib(
    libcockatrice_game
    LIBS libcockatrice_card libcockatrice_deck_list libcockatrice_filters libcockatrice_interfaces libcockatrice_network
         libcockatrice_protocol libcockatrice_settings libcockatrice_utility Qt6::Core Qt6::Concurrent Qt6::Gui
  )
endif()
cockatrice_fuzzer(fuzz_replay SOURCES ${VERSION_STRING_CPP} LIBS libcockatrice_game Qt6::Core)
