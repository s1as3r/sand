#!/usr/bin/env bash
BUILD_PATH="build"
SRC="../src"
EXE_NAME="sand"
COMPILER="gcc"

# debug flags
DEBUG=(
    "-g"
)

# compile time defines
DEFINES=()

# linux platform libraries
LIBS=(
    "-lraylib"
    "-lm"
    "-lpthread"
    "-ldl"
    "-lX11"
    "-lGL"
    "-lrt"
)

# compiler flags
COMP_FLAGS=(
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Og"
    "-std=c11"
)

# Build commands
EXE_CMD=("$COMPILER" "${DEFINES[@]}" "${DEBUG[@]}" "${COMP_FLAGS[@]}" \
         "-o" "$EXE_NAME" "$SRC/main.c" "${LIBS[@]}")

EXE_CMD_STR=$(IFS=' '; echo "${EXE_CMD[*]}")

# Create build directory if it doesn't exist
if [[ ! -d "$BUILD_PATH" ]]; then
    echo "Created Build Directory"
    mkdir -p "$BUILD_PATH"
fi

cd "$BUILD_PATH" || exit
echo "===== $EXE_NAME ====="
echo "$EXE_CMD_STR"
eval "$EXE_CMD_STR"
cd ..
