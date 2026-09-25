# Pre-build script for [env:native]. The native platform calls plain gcc/g++
# from PATH; Windows has none by default, so put PlatformIO's MinGW package
# (platform_packages = platformio/toolchain-gccmingw32) first on PATH.
import os

Import("env")

toolchain = env.PioPlatform().get_package_dir("toolchain-gccmingw32")
if toolchain and os.path.isdir(os.path.join(toolchain, "bin")):
    env.PrependENVPath("PATH", os.path.join(toolchain, "bin"))

# Compile C++ as the firmware does (arm-none-eabi-g++ 7.2 defaults to
# gnu++14); MinGW g++ 5.1 would otherwise default to C++98. C++ only, so
# Unity's C sources are not given a C++ -std flag.
env.Append(CXXFLAGS=["-std=gnu++14"])

# Link the GCC/C++ runtime statically. Otherwise the 32-bit test program
# loads libstdc++-6.dll from PATH at run time and can pick up a 64-bit copy
# (e.g. Git for Windows' mingw64\bin), failing with 0xC000007B.
env.Append(LINKFLAGS=["-static", "-static-libgcc", "-static-libstdc++"])
