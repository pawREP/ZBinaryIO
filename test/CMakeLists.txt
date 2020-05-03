set(TEST_NAME ${CMAKE_PROJECT_NAME}Test)

set(TEST_SOURCES 
	main.cpp
	ZBinaryReaderTest.cpp
	ZBinaryWriterTest.cpp
)

add_executable(${TEST_NAME} ${TEST_SOURCES})

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

target_link_libraries(${TEST_NAME} PUBLIC coverage_config)
target_link_libraries(${TEST_NAME} PUBLIC ZBinaryReader gtest)

# add_custom_command(
	# TARGET ${TEST_NAME} POST_BUILD
    # COMMAND ${CMAKE_COMMAND} -E copy
# "${CMAKE_CURRENT_SOURCE_DIR}/testData/testData.bin"
# "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/testData/testData.bin"
  # )