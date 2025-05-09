# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr_yaml_test)
target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

set_property(GLOBAL PROPERTY EXPECTED_ERROR 0)

macro(message)
  if(DEFINED expect_failure)
  if(${ARGV0} STREQUAL FATAL_ERROR)
    if("${ARGV1}" STREQUAL "${expect_failure}")
      # This is an expected error.
      get_property(error_count GLOBAL PROPERTY EXPECTED_ERROR)
      math(EXPR error_count "${error_count} + 1")
      set_property(GLOBAL PROPERTY EXPECTED_ERROR ${error_count})
      return()
    else()
      _message("Unexpected error occurred")
    endif()
    endif()
  endif()
  _message(${ARGN})
endmacro()

macro(test_assert)
  cmake_parse_arguments(TA_ARG "" "COMMENT" "TEST" ${ARGN})
  if(${TA_ARG_TEST})
    message("${CMAKE_CURRENT_FUNCTION}(): Passed")
  else()
    message(FATAL_ERROR
      "${CMAKE_CURRENT_FUNCTION}(): Failed\n"
      "Test: ${TA_ARG_TEST}\n"
      "${TA_ARG_COMMENT}"
    )
  endif()
endmacro()

function(test_reading_string)
  set(expected "Simple string")
  yaml_get(actual NAME yaml-test KEY cmake test key-string)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_list_strings)
  set(expected 4)
  yaml_length(actual NAME yaml-test KEY cmake test key-list-string)
  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml list length does not match expectation."
  )

  set(expected "a" "list" "of" "strings")
  yaml_get(actual NAME yaml-test KEY cmake test key-list-string)

  foreach(a e IN ZIP_LISTS actual expected)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_reading_int)
  set(expected 42)
  yaml_get(actual NAME yaml-test KEY cmake test key-int)

  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_list_int)
  set(expected 3)
  yaml_length(actual NAME yaml-test KEY cmake test key-list-int)
  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml list length does not match expectation."
  )

  set(expected 4 10 2)
  yaml_get(actual NAME yaml-test KEY cmake test key-list-int)

  foreach(a e IN ZIP_LISTS actual expected)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_reading_int)
  set(expected 42)
  yaml_get(actual NAME yaml-test KEY cmake test key-int)

  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_map_list_entry)
  set(expected_length 2)
  set(expected_name "MapEntry1")
  set(expected_int  5)
  yaml_length(actual_length NAME yaml-test KEY cmake test map-list)
  yaml_get(actual_name NAME yaml-test KEY cmake test map-list 0 map-entry-name)
  yaml_get(actual_int NAME yaml-test KEY cmake test map-list 0 map-entry-int)

  test_assert(TEST ${expected_length} EQUAL ${actual_length}
              COMMENT "yaml key value does not match expectation."
  )
  test_assert(TEST ${expected_name} STREQUAL ${actual_name}
              COMMENT "yaml key value does not match expectation."
  )
  test_assert(TEST ${expected_int} EQUAL ${actual_int}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_not_found)
  set(expected cmake-missing-NOTFOUND)
  yaml_get(actual NAME yaml-test KEY cmake missing test key)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_reading_not_found_array)
  set(expected cmake-missing-NOTFOUND)
  yaml_length(actual NAME yaml-test KEY cmake missing test array list)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_reading_not_array)
  set(expected -1)
  yaml_length(actual NAME yaml-test KEY cmake test key-int)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Not array expected, so length should be -1."
  )
endfunction()

function(test_reading_not_found_map_list_entry)
  set(expected cmake-test-map-list-3-NOTFOUND)
  yaml_get(actual NAME yaml-test KEY cmake test map-list 3 map-entry-name)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_save_new_file)
  yaml_save(NAME yaml-test FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_save.yaml)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_save.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )
  set(expected "Simple string")
  yaml_get(actual NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test key-string)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )

  set(expected 42)
  yaml_get(actual NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test key-int)

  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_setting_string)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value "A new string")
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-string VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-string)

  test_assert(TEST ${new_value} STREQUAL ${readback}
              COMMENT "new yaml value does not match readback value."
  )
endfunction()

function(test_setting_list_strings)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value "A" "new" "list" "of" "strings")
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  test_assert(TEST 5 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_int)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value 42)
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)

  test_assert(TEST ${new_value} STREQUAL ${readback}
              COMMENT "new yaml value does not match readback value."
  )
endfunction()

function(test_setting_list_int)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value 42 41 40 2 10)
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-int LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  test_assert(TEST 5 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_map_list_entry)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_entry_name_0 MapEntryNew1)
  set(new_entry_int_0  42)
  set(new_entry_name_1 MapEntryNew2)
  set(new_entry_int_1  24)
  set(new_entry_name_2 MapEntryNew3)
  set(new_entry_int_2  4224)
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set map-list LIST
           MAP "map-entry-name: ${new_entry_name_0}, map-entry-int: ${new_entry_int_0}"
           MAP "map-entry-name: ${new_entry_name_1}, map-entry-int: ${new_entry_int_1}"
           MAP "map-entry-name: ${new_entry_name_2}, map-entry-int: ${new_entry_int_2}"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the values.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list)

  test_assert(TEST 3 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  foreach(index 0 1 2)
    yaml_get(readback_name NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-name)
    yaml_get(readback_int  NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-int)

    test_assert(TEST "${readback_name}" STREQUAL "${new_entry_name_${index}}"
                COMMENT "list values mismatch."
    )
    test_assert(TEST "${readback_int}" EQUAL "${new_entry_int_${index}}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_map_list_entry_windows)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_entry_name_0 MapEntryWindowsPath1)
  set(new_entry_path_0  "c:/tmp/zephyr")
  set(new_entry_name_1 MapEntryWindowsPath2)
  set(new_entry_path_1  "c:/program files/space")
  set(new_entry_name_2 MapEntryWindowsPath3)
  set(new_entry_path_2  "D:/alternative/drive")
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set map-list LIST
           MAP "map-entry-name: ${new_entry_name_0}, map-entry-path: ${new_entry_path_0}"
           MAP "map-entry-name: ${new_entry_name_1}, map-entry-path: ${new_entry_path_1}"
           MAP "map-entry-name: ${new_entry_name_2}, map-entry-path: ${new_entry_path_2}"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the values.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list)

  test_assert(TEST 3 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  foreach(index 0 1 2)
    yaml_get(readback_name NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-name)
    yaml_get(readback_path  NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-path)

    test_assert(TEST "${readback_name}" STREQUAL "${new_entry_name_${index}}"
                COMMENT "list values mismatch."
    )
    test_assert(TEST "${readback_path}" STREQUAL "${new_entry_path_${index}}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_map_list_entry_commas)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_entry_name_0 TestString1)
  set(new_entry_str_0  "'A\\,string'")
  set(new_entry_name_1 TestString2)
  set(new_entry_str_1  "'\\, is first'")
  set(new_entry_name_2 TestString3)
  set(new_entry_str_2  "'\\, and : is\\,everywhere\\,'")
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set map-list LIST
           MAP "map-entry-name: ${new_entry_name_0}, map-entry-str: ${new_entry_str_0}"
           MAP "map-entry-name: ${new_entry_name_1}, map-entry-str: ${new_entry_str_1}"
           MAP "map-entry-name: ${new_entry_name_2}, map-entry-str: ${new_entry_str_2}"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the values.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list)

  test_assert(TEST 3 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  foreach(index 0 1 2)
    yaml_get(readback_name NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-name)
    yaml_get(readback_str  NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-str)

    test_assert(TEST "${readback_name}" STREQUAL "${new_entry_name_${index}}"
                COMMENT "list values mismatch."
    )
    test_assert(TEST "'${readback_str}'" STREQUAL "${new_entry_str_${index}}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()
function(test_setting_empty_value)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value)
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)

  if(DEFINED readback)
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}(): Failed\n"
                        "Empty value expected, but got: ${readback}"
    )
  else()
    message("${CMAKE_CURRENT_FUNCTION}(): Passed")
  endif()
endfunction()

function(test_setting_empty_list)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-int LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  test_assert(TEST 0 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_set_remove_int)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value 42)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)

  test_assert(TEST ${new_value} STREQUAL ${readback}
              COMMENT "new yaml value does not match readback value."
  )

  # Remove the setting and write the file again.
  yaml_remove(NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)
  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_readback)

  # Read-back again and verify the value has been removed.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback_removed
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback_removed KEY cmake test set key-int)

  set(expected cmake-test-set-key-int-NOTFOUND)

  test_assert(TEST ${expected} STREQUAL ${readback}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_fail_missing_filename)
  yaml_create(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  set(new_value 42)
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  set(expect_failure "yaml_save(...) missing a required argument: FILE")
  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  get_property(errors GLOBAL PROPERTY EXPECTED_ERROR)
  test_assert(TEST 1 EQUAL ${errors}
              COMMENT "No error occurred when error was expected.\nExpected error: ${expect_failure}"
  )
  set_property(GLOBAL PROPERTY EXPECTED_ERROR 0)
endfunction()

yaml_load(FILE ${CMAKE_CURRENT_LIST_DIR}/test.yaml NAME yaml-test)
test_reading_string()
test_reading_int()
test_reading_list_strings()
test_reading_list_int()
test_reading_map_list_entry()
test_reading_not_found()
test_reading_not_found_array()
test_reading_not_array()
test_reading_not_found_map_list_entry()

test_save_new_file()

test_setting_int()
test_setting_string()
test_setting_list_strings()
test_setting_list_int()
test_setting_map_list_entry()
test_setting_map_list_entry_windows()
test_setting_map_list_entry_commas()

test_setting_empty_value()
test_setting_empty_list()

test_set_remove_int()

test_fail_missing_filename()

target_sources(app PRIVATE src/main.c)
