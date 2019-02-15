Windows build cmd:
gn gen out/x86/Debug --args='is_debug=true target_cpu=\"x86\" rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false rtc_include_tests=false rtc_build_examples=false rtc_libvpx_build_vp9=false rtc_include_ilbc=false' --ide=vs2017
gn gen out/x86/Release --args='is_debug=false target_cpu=\"x86\" rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false rtc_include_tests=false rtc_build_examples=false rtc_libvpx_build_vp9=false rtc_include_ilbc=false' --ide=vs2017

Linux build cmd:
Debug module:
gn gen -C out/Debug  --args="is_debug=true target_cpu=\"x64\" rtc_include_tests=false rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false rtc_include_ilbc=false"

Release:
gn gen -C out/Linux/Release  --args="is_debug=false target_cpu=\"x64\" rtc_include_tests=false rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false rtc_include_ilbc=false"

Arm:
gn gen out/arm/Release --args='is_debug=false is_clang=false target_os="linux" target_cpu="arm" treat_warnings_as_errors=false rtc_include_tests=false use_custom_libcxx=false use_ozone=true rtc_build_examples=false rtc_use_x11=false rtc_include_pulse_audio=false rtc_libvpx_build_vp9=false rtc_use_h264=false rtc_include_ilbc=false rtc_include_opus=true use_sysroot=true target_sysroot="/home/gobert/develop/rv1108_glibc_toolschain/toolschain/usr/arm-buildroot-linux-gnueabihf/sysroot"'

build_without_clang:

with_h264:
gn gen -C out/linux/Release  --args="is_debug=false target_cpu=\"x64\" rtc_include_tests=false rtc_use_h264=true rtc_initialize_ffmpeg=true ffmpeg_branding=\"Chrome\" is_component_build=false is_clang=false treat_warnings_as_errors=false use_custom_libcxx=false strip_debug_info=true use_rtti=false"

without_h264:
gn gen -C out/linux/Release  --args="is_debug=false target_cpu=\"x64\" rtc_include_tests=false is_component_build=false is_clang=false treat_warnings_as_errors=false use_custom_libcxx=false use_rtti=false rtc_use_x11=false rtc_include_pulse_audio=false rtc_libvpx_build_vp9=false rtc_use_h264=false rtc_include_ilbc=false rtc_include_opus=true"
