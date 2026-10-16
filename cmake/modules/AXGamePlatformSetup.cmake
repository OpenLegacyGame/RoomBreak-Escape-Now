if(APPLE)
  set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
  set_xcode_property(${APP_NAME} INSTALL_PATH "\$(LOCAL_APPS_DIR)")
  set_xcode_property(${APP_NAME} PRODUCT_BUNDLE_IDENTIFIER "local.roombreak.rebuilt")

  if(MACOSX)
    set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/mac/Info.plist")
  elseif(TVOS)
    set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/ios/targets/tvos/Info.plist")
    set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "Brand Assets")
  elseif(IOS)
    set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/ios/targets/ios/Info.plist")
    set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    set_xcode_property(${APP_NAME} TARGETED_DEVICE_FAMILY "1,2")
  endif()

# For code-signing, set the DEVELOPMENT_TEAM:
elseif(WINDOWS)
  # config quick starter batch script run.bat for windows
  if(WIN32)
    file(RELATIVE_PATH CMAKE_BUILD_RELATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/run.bat.in" "${PROJECT_BINARY_DIR}/run.bat" @ONLY)
  endif()

  if(NOT _AX_USE_PREBUILT)
    ax_sync_target_dlls(${APP_NAME})
  endif()
endif()

if (_AX_TESTS)
  if(MACOSX)
    set_target_properties(${TEST_TARGET_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
    set_xcode_property(${TEST_TARGET_NAME} INSTALL_PATH "\$(LOCAL_APPS_DIR)")
    set_xcode_property(${TEST_TARGET_NAME} PRODUCT_BUNDLE_IDENTIFIER "local.roombreak.rebuilt-test")
    set_target_properties(${TEST_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/mac/Info.plist")

    # For code-signing, set the DEVELOPMENT_TEAM:
  elseif(WINDOWS)
    if(NOT _AX_USE_PREBUILT)
      ax_sync_target_dlls(${TEST_TARGET_NAME})
    endif()
  endif()
endif()
