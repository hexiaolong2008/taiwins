################################################################################
# library
################################################################################

include(Wayland)

WAYLAND_ADD_PROTOCOL_CLIENT(proto_twshell_client
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-shell.xml"
  taiwins-shell
  )

WAYLAND_ADD_PROTOCOL_SERVER(proto_twshell_server
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-shell.xml"
  taiwins-shell
  )

WAYLAND_ADD_PROTOCOL_CLIENT(proto_twconsole_client
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-console.xml"
  taiwins-console
  )

WAYLAND_ADD_PROTOCOL_SERVER(proto_twconsole_server
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-console.xml"
  taiwins-console
  )

WAYLAND_ADD_PROTOCOL_CLIENT(proto_twtheme_client
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-theme.xml"
  taiwins-theme
  )

WAYLAND_ADD_PROTOCOL_SERVER(proto_twtheme_server
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-theme.xml"
  taiwins-theme
  )

add_library(protos-client STATIC
  ${proto_twtheme_client}
  ${proto_twshell_client}
  ${proto_twconsole_client}
  )

add_library(protos-server STATIC
  ${proto_twshell_server}
  ${proto_twconsole_server}
  ${proto_twtheme_server}
  )

target_include_directories(protos-client PUBLIC
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
target_include_directories(protos-server PUBLIC
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

################################################################################
# install
################################################################################
include(GNUInstallDirs)
install(FILES
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-theme.xml"
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-console.xml"
  "${TAIWINS_PROTOCOLS_DIR}/taiwins-shell.xml"
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/taiwins/protocols)
