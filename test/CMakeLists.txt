find_package(nlohmann_json CONFIG REQUIRED)
add_executable(aoi_test aoi_test.cpp)
target_link_libraries(aoi_test PRIVATE ${PROJECT_NAME} nlohmann_json::nlohmann_json)