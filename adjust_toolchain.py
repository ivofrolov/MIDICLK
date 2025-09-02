Import("env")
Import("projenv")

# binaries in toolchain-atmelavr and tool-avrdude packages on macos are only compatible
# with x86_64 architecture, so you either need to have rosetta installed or just use
# compiler and uploader from homebrew
if env["HOST_OS"] == "darwin" and env["HOST_ARCH"] == "arm64":
    tools = {
        "HOMEBREW_PREFIX": env["ENV"].get("HOMEBREW_PREFIX", "/opt/homebrew"),
        # compiler
        "AR": "$HOMEBREW_PREFIX/bin/avr-gcc-ar",
        "AS": "$HOMEBREW_PREFIX/bin/avr-as",
        "CC": "$HOMEBREW_PREFIX/bin/avr-gcc",
        "CXX": "$HOMEBREW_PREFIX/bin/avr-g++",
        "GDB": "$HOMEBREW_PREFIX/bin/avr-gdb",
        "OBJCOPY": "$HOMEBREW_PREFIX/bin/avr-objcopy",
        "RANLIB": "$HOMEBREW_PREFIX/bin/avr-gcc-ranlib",
        "SIZETOOL": "$HOMEBREW_PREFIX/bin/avr-size",
        # uploader
        "FUSESUPLOADER": "$HOMEBREW_PREFIX/bin/avrdude",
        "FUSESUPLOADERFLAGS": [
            "-p",
            "$BOARD_MCU",
            "-C",
            "$HOMEBREW_PREFIX/etc/avrdude.conf",
            "-c",
            "$UPLOAD_PROTOCOL",
        ],
        "UPLOADER": "$HOMEBREW_PREFIX/bin/avrdude",
        "UPLOADERFLAGS": [
            "-v",
            "-p",
            "$BOARD_MCU",
            "-C",
            "$HOMEBREW_PREFIX/etc/avrdude.conf",
            "-c",
            "$UPLOAD_PROTOCOL",
        ],
    }
    env.Replace(**tools)
    projenv.Replace(**tools)
