package com.winlator.core;

import static com.winlator.container.Container.STEAM_TYPE_NORMAL;

import com.winlator.container.Container;

public abstract class DefaultVersion {

    public static final String BOX86 = "0.3.2";
    public static final String BOX64 = "0.3.6";
    public static final String FEXCORE = "2603";
    public static String WRAPPER = "System";
    public static final String TURNIP = "25.2.0";
    public static final String ZINK = "22.2.5";
    public static final String VIRGL = "23.1.9";
    public static String DXVK = "2.6.1-gplasync";
    public static final String D8VK = "1.0";
    public static String VKD3D = "2.14.1";
    public static final String CNC_DDRAW = "6.6";
    public static final String VORTEK = "2.1-22.2.5";
    public static final String ADRENO = "819.2";
    public static final String SD8ELITE = "800.51";
    // ANGLE driver: Google ANGLE (chromium branch 7748) — BSD-3-Clause licensed
    public static final String ANGLE = "7748";
    // GL4ES: GL→GLES translation shim — MIT licensed
    // NOTE: must match the actual asset filename in app/src/main/assets/graphics_driver/
    // Current asset on disk: gl4es-bionic-1.1.7.tzst (Bionic/Android build with real GLX support)
    // If a new rebuild is packaged, bump this and add the new .tzst to assets simultaneously.
    public static final String GL4ES = "1.1.7";
    public static String STEAM_TYPE = STEAM_TYPE_NORMAL;
    public static String VARIANT = Container.GLIBC;
    public static String DEFAULT_GRAPHICS_DRIVER = "vortek";
    public static String WINE_VERSION = com.winlator.core.WineInfo.MAIN_WINE_VERSION.identifier();
    public static String ASYNC = "1";
    public static String ASYNC_CACHE = "0";
}
