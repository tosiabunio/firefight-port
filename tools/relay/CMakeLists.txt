# The relay server (phases 7 and 9): forwards game sessions between players who connect out to it,
# over ENet (desktop) or WebSocket (browser). Its protocol is source/engine/1ee/1ee_relay.h.
add_executable(ff_relay relay.cpp)
target_include_directories(ff_relay PRIVATE ${CMAKE_SOURCE_DIR}/source/engine/1ee)
target_link_libraries(ff_relay PRIVATE ff::enet)
ff_modern_options(ff_relay)
