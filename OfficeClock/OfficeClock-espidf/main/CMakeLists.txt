set(ESPlib ${COMPONENT_DIR}/../../../ESPlib)
set(esplibFiles
    Application.cpp JsonStreamingParser.cpp BrightnessManager.cpp ButtonManager.cpp LittleFSShim.cpp 
    Clock.cpp Graphics.cpp HTTPParser.cpp LuaManager.cpp Max7219Display.cpp System.cpp TimeWeatherServer.cpp 
    WebFileSystem.cpp Shell.cpp IDFWiFiPortal.cpp LuaWFS.cpp HTTPFetchClient.cpp
)
list(TRANSFORM esplibFiles PREPEND ${ESPlib}/)

set(OfficeClock ${COMPONENT_DIR}/../../)
set(officeClockFiles OfficeClock.cpp)
list(TRANSFORM officeClockFiles PREPEND ${OfficeClock}/)

set(Lua ${ESPlib}/lua/lua-5.4.8/src/)
set(luaFiles lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c
    ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c
    loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c lundump.c
    lutf8lib.c lvm.c lzio.c)
list(TRANSFORM luaFiles PREPEND ${Lua}/)

idf_component_register(SRCS "main.cpp" ${officeClockFiles} ${esplibFiles} ${luaFiles}
                    PRIV_REQUIRES esp_adc esp_driver_gpio esp_wifi spi_flash nvs_flash esp_http_server dns_server esp_timer esp_driver_tsens esp_http_client app_update
                    INCLUDE_DIRS "." ${ESPlib} ${OfficeClock} ${Lua})

target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-missing-field-initializers)
