# Повышаем версию до актуальной (C++14 полноценно поддерживается с 3.1)
cmake_minimum_required(VERSION 3.12)
project(PBXMTV508 CXX C)

# Настройки стандарта C++
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Настройка путей для модулей CMake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

# Настройка флагов компилятора для разных режимов
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Для Debug используем -O0 или -Og, чтобы работала отладка
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -O0")
    
    # Для Release оставляем максимальную оптимизацию
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
endif()

# Определение версии платы
set(BOARD_REV "0" CACHE STRING "Board rev (0 - PBX-MTV-508, 1 - PN-MTV-581)")
message(STATUS "Board rev: ${BOARD_REV}")

# Поиск зависимостей (все Qt компоненты теперь REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets Network WebSockets Svg SerialPort REQUIRED)
find_package(Threads REQUIRED)
find_package(Zvbi REQUIRED)

# Подключение поддиректорий
add_subdirectory(zvbi-test)
add_subdirectory(profnext)
add_subdirectory(tsl)

# --- Исполняемый файл 1: pbx-mtv-508 ---
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

# Индивидуальные макросы для конкретной цели
target_compile_definitions(pbx-mtv-508 PRIVATE 
    BOARD_REV=${BOARD_REV}
    QT_NO_EXCEPTIONS
)

# Пути к локальным заголовкам (заменяет старый include_directories)
target_include_directories(pbx-mtv-508 PRIVATE 
    tsl
    ${ZVBI_INCLUDE_DIRS}
)

# Линковка библиотек (Qt тянет свои заголовки автоматически)
target_link_libraries(pbx-mtv-508 PRIVATE 
    Qt5::Widgets 
    Qt5::Core 
    Qt5::Network
    Qt5::WebSockets 
    Qt5::Svg 
    Qt5::SerialPort 
    ${ZVBI_LIBRARY} 
    tsl-server
    Threads::Threads
)

# --- Исполняемый файл 2: rgbyuv-test ---
add_executable(rgbyuv-test rgbyuv-test/rgbyuv-test.cpp)
target_link_libraries(rgbyuv-test PRIVATE Qt5::Widgets)

# --- Исполняемый файл 3: read_mem ---
add_executable(read_mem read_mem/read_mem.cpp)
target_link_libraries(read_mem PRIVATE Qt5::Widgets Qt5::Core)

# --- Исполняемый файл 4: devmem ---
add_executable(devmem devmem/devmem2.c)

# --- Правила инсталляции (Исправлено с PROGRAMS на TARGETS) ---
install(TARGETS pbx-mtv-508 rgbyuv-test read_mem devmem DESTINATION bin)

#edited by II