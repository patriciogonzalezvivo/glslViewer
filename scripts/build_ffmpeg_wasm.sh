#!/bin/bash
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
# Project root is parent of scripts directory
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Directory to install FFmpeg into (relative to project root)
INSTALL_DIR=$PROJECT_ROOT/deps/ffmpeg/install
BUILD_DIR=$PROJECT_ROOT/deps/ffmpeg/build
SOURCE_DIR=$PROJECT_ROOT/deps/ffmpeg/src

mkdir -p $INSTALL_DIR
mkdir -p $BUILD_DIR
mkdir -p $SOURCE_DIR

# Check if emcc is in path
if ! command -v emcc &> /dev/null
then
    echo "emcc could not be found. Please accept the Emscripten environment variables (source emsdk_env.sh)."
    exit 1
fi

cd $SOURCE_DIR

# Clone FFmpeg if not present
if [ ! -d "ffmpeg" ]; then
    echo "Cloning FFmpeg..."
    git clone --depth 1 --branch n5.1 https://github.com/FFmpeg/FFmpeg.git ffmpeg
else
    echo "FFmpeg source found."
fi

cd ffmpeg

# Configure FFmpeg for Emscripten
# Disabling many features to keep size small and reduce dependency complexity
echo "Configuring FFmpeg..."
emconfigure ./configure \
    --prefix=$INSTALL_DIR \
    --cc="emcc" \
    --cxx="em++" \
    --ar="emar" \
    --nm="emnm" \
    --ranlib="emranlib" \
    --target-os=none \
    --arch=x86_32 \
    --enable-cross-compile \
    --disable-x86asm \
    --disable-inline-asm \
    --disable-stripping \
    --disable-programs \
    --disable-doc \
    --disable-debug \
    --disable-runtime-cpudetect \
    --disable-autodetect \
    --disable-pthreads \
    --disable-os2threads \
    --disable-w32threads \
    --disable-network \
    --enable-gpl \
    --enable-version3 \
    --disable-everything \
    --enable-decoder=h264 \
    --enable-decoder=hevc \
    --enable-decoder=vp8 \
    --enable-decoder=vp9 \
    --enable-decoder=aac \
    --enable-decoder=mp3 \
    --enable-decoder=mjpeg \
    --enable-decoder=png \
    --enable-parser=h264 \
    --enable-parser=hevc \
    --enable-parser=vp8 \
    --enable-parser=vp9 \
    --enable-parser=aac \
    --enable-parser=mpegaudio \
    --enable-demuxer=mov \
    --enable-demuxer=matroska \
    --enable-demuxer=image2 \
    --enable-demuxer=mp3 \
    --enable-demuxer=aac \
    --enable-protocol=file \
    --enable-filter=scale \
    --enable-avcodec \
    --enable-avformat \
    --enable-avutil \
    --enable-swscale

# Build
echo "Building FFmpeg..."
emmake make -j$(nproc)

# Install
echo "Installing FFmpeg..."
emmake make install

echo "Done! FFmpeg installed to $INSTALL_DIR"
