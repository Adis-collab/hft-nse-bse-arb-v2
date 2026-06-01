cmake_minimum_required(VERSION 3.20)
project(nse_bse_hft_arb VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HFT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(HFT_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
endif()

include_directories(src)

set(HFT_CORE_SOURCES
  src/common/types.cpp
  src/common/csv.cpp
  src/common/config.cpp
  src/md/replay_reader.cpp
  src/md/csv_parser.cpp
  src/md/market_data_handler.cpp
  src/book/order_book.cpp
  src/book/book_manager.cpp
  src/strategy/cost_model.cpp
  src/strategy/arbitrage_strategy.cpp
  src/risk/risk_config.cpp
  src/risk/position_book.cpp
  src/risk/risk_engine.cpp
  src/oms/order_manager.cpp
  src/hedge/pair_trade.cpp
  src/hedge/hedge_policy.cpp
  src/sim/scenario.cpp
  src/sim/matching_engine.cpp
  src/sim/simulator.cpp
  src/metrics/latency_recorder.cpp
)

add_library(hft_core ${HFT_CORE_SOURCES})
target_include_directories(hft_core PUBLIC src)

add_executable(hft_demo apps/hft_demo.cpp)
target_link_libraries(hft_demo PRIVATE hft_core)

add_executable(scenario_suite apps/scenario_suite.cpp)
target_link_libraries(scenario_suite PRIVATE hft_core)

add_executable(bench_pipeline bench/bench_pipeline.cpp)
target_link_libraries(bench_pipeline PRIVATE hft_core)

add_executable(bench_queue_pipeline bench/bench_queue_pipeline.cpp)
target_link_libraries(bench_queue_pipeline PRIVATE hft_core)

add_executable(hft_tests tests/test_main.cpp)
target_link_libraries(hft_tests PRIVATE hft_core)

enable_testing()
add_test(NAME hft_tests COMMAND hft_tests)
set_tests_properties(hft_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
