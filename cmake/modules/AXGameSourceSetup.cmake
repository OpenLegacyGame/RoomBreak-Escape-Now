if(ANDROID)
  # the APP_NAME should match on AndroidManifest.xml
  list(APPEND GAME_SOURCE
    platforms/android/app/jni/main.cpp
  )
elseif(LINUX)
  list(APPEND GAME_SOURCE
    platforms/linux/main.cpp
  )
  list(APPEND GAME_SOURCE ${common_content_files})
elseif(WASM)
  list(APPEND GAME_SOURCE
    platforms/web/main.cpp
  )
  list(APPEND GAME_SOURCE ${common_content_files})
elseif(WINDOWS)
  if(NOT WINRT)
    list(APPEND GAME_HEADER
      platforms/windows/main.h
      platforms/windows/resource.h
    )
    list(APPEND GAME_SOURCE
      platforms/windows/main.cpp
      platforms/windows/game.rc
    )
  else()
    ax_setup_winrt_sources()
  endif()

  list(APPEND GAME_SOURCE ${common_content_files})
elseif(APPLE)
  if(IOS)
    list(APPEND GAME_HEADER
      platforms/apple/ios/GameAppController.h
      platforms/apple/ios/GameViewController.h
      platforms/apple/ios/GameSceneDelegate.h
    )

    if(TVOS)
      set(APP_UI_RES
        platforms/apple/ios/targets/tvos/LaunchScreenBackground.png
        platforms/apple/ios/targets/tvos/LaunchScreen.storyboard
        platforms/apple/ios/targets/tvos/Images.xcassets
      )
    else()
      set(APP_UI_RES
        platforms/apple/ios/targets/ios/LaunchScreenBackground.png
        platforms/apple/ios/targets/ios/LaunchScreen.storyboard
        platforms/apple/ios/targets/ios/Images.xcassets
      )
    endif()

    list(APPEND GAME_SOURCE
      platforms/apple/ios/main.mm
      platforms/apple/ios/GameAppController.mm
      platforms/apple/ios/GameViewController.mm
      platforms/apple/ios/GameSceneDelegate.mm
      platforms/apple/ios/Prefix.pch
      ${APP_UI_RES}
    )
  elseif(MACOSX)
    set(APP_UI_RES
      platforms/apple/mac/Icon.icns
    )
    list(APPEND GAME_SOURCE
      platforms/apple/mac/main.cpp
      platforms/apple/mac/Prefix.pch
      ${APP_UI_RES}
    )
  endif()

  list(APPEND GAME_SOURCE ${common_content_files})
endif()
