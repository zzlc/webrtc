Debug module:
ninja -C out/Debug  --args="is_debug=true target_cpu=\"x64\" rtc_include_tests=false rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false"

Release:
gn gen -C out/Linux/Release  --args="is_debug=false target_cpu=\"x64\" rtc_include_tests=false rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false"

Arm:
gn gen out/Arm/Release --args='is_debug=false is_clang=false target_os="linux" target_cpu="arm" treat_warnings_as_errors=false rtc_include_tests=false use_custom_libcxx=false use_ozone=true rtc_build_examples=false rtc_use_x11=false rtc_include_pulse_audio=false use_sysroot=true target_sysroot="/home/gobert/develop/rv1108_toolchain_uclibc/toolschain/usr/arm-rkcvr-linux-uclibcgnueabihf/sysroot"'

build_without_clang:
gn gen -C out/Linux/Release  --args="is_debug=false target_cpu=\"x64\" rtc_include_tests=false rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\"
is_component_build=false is_clang=false treat_warnings_as_errors=false use_custom_libcxx=false strip_debug_info=false use_rtti=true"
