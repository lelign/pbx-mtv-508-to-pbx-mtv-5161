cmake_minimum_required(VERSION 2.8.11)
project(PBXMTV508)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
        add_definitions(-DQT_NO_EXCEPTIONS)
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

# 0 - PBX-MTV-508
# 1 - PN-MTV-581
set(BOARD_REV "0" CACHE STRING "Board rev")
add_definitions(-DBOARD_REV=${BOARD_REV})
message("Board rev: ${BOARD_REV}")

find_package(Qt5Network)
find_package(Qt5WebSockets)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5SerialPort)
find_package(Threads)
#FIND_PACKAGE(libusb-1.0 REQUIRED)
#find_package(bitstream REQUIRED)
find_package(Zvbi REQUIRED)

find_package(Qt5Svg REQUIRED)
include_directories(${Qt5Svg_INCLUDE_DIRS})

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})
include_directories(${Qt5WebSockets_INCLUDE_DIRS})
#include_directories(${LIBUSB_1_INCLUDE_DIRS})
#include_directories(${LIBBITSTREAM_INCLUDE_DIR})
include_directories(${ZVBI_INCLUDE_DIRS})
include_directories(tsl)
SET (CMAKE_POSITION_INDEPENDENT_CODE ON)
SET (CMAKE_AUTORCC ON)

add_subdirectory(zvbi-test)
add_subdirectory(profnext)
add_subdirectory(tsl)

add_executable(pbx-mtv-508
        pbx-mtv-508-main.cpp
        pbx-mtv-508.cpp
        mtv-system/mtv-system.cpp
	hdmi_adv7513/hdmi_adv7513.cpp
	i2c/i2c.cpp	
	web_socket_server/websocketserver.cpp
	mtv-web/mtv_web.cpp
	hardware_diagnostics/hardware_diagnostics.cpp
        layout/layout.cpp
        cascade_server/cascade_server.cpp
        cascade_ctrl/cascade_ctrl.cpp
#        m26/mb86m26.cpp
#        m26/mb86m26_control.cpp
#        m26/mb86m26_lib.cpp
#        m26/mpeg-dump.cpp
#        HlsServer/hlsserver.cpp
	led/led_class.cpp
	factory_defaults/factory_defaults.cpp
	ip-utils/ip-utils.c
        uptime-utils/uptime_utils.cpp
	dns-utils/dns-utils.c
        watchdog.cpp
	qpps/qpps.cpp
        time_counter/time_counter.cpp
	utils/utils.c
	ntpdate-utils/ntpdate-utils.c
	gpio/gpio.cpp
	eventlog-api/eventlog.cpp
        resource.qrc
        mtv-system/anc-reader.cpp
        ipout/ts-in-reader.cpp
        ipout/ts-reader.cpp
	file_handle_leaks/file_handle_leaks.cpp
	scte_104/scte_104.cpp
	snmp_agent/snmp_agent.cpp
        mtv-snmp/mtv_snmp.cpp
        teletext-decoder/teletext-decoder.cpp
        profitt-security/profitt-security.c
        profnext/5566.c
        profnext/profnext-frontpanel.cpp
        ctrl_panel_PERP4116/ctrl_panel_perp4116.cpp
        vgpi_server/vgpi_server.cpp
        vgpi_api/vgpi_api.cpp
        remote_ctrl_preset/remote_ctrl_preset.cpp
)
#target_link_libraries(pbx-mtv-508 Qt5::Widgets Qt5::Core Qt5::WebSockets Qt5::Svg Qt5::SerialPort ${LIBUSB_1_LIBRARIES} ${ZVBI_LIBRARY} tsl-server)
target_link_libraries(pbx-mtv-508 Qt5::Widgets Qt5::Core Qt5::WebSockets Qt5::Svg Qt5::SerialPort ${ZVBI_LIBRARY} tsl-server)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/pbx-mtv-508 DESTINATION bin)

#add_executable(m26-eeprom
#        m26/m26-eeprom.cpp
#)
#target_link_libraries(m26-eeprom ${LIBUSB_1_LIBRARIES})

#INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/m26-eeprom DESTINATION bin)

add_executable(rgbyuv-test
        rgbyuv-test/rgbyuv-test.cpp
)
target_link_libraries(rgbyuv-test Qt5::Widgets)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/rgbyuv-test DESTINATION bin)

add_executable(read_mem
        read_mem/read_mem.cpp
)
target_link_libraries(read_mem Qt5::Widgets Qt5::Core)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/read_mem DESTINATION bin)

add_executable(devmem
        devmem/devmem2.c
)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/devmem DESTINATION bin)

