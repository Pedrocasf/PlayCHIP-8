# PlayCHIP-8
CHIP-8 emulator for the Playdate
# Compile with:
  cmake -DCMAKE_TOOLCHAIN_FILE=$PLAYDATE_SDK_PATH/C_API/buildsupport/arm.cmake .. --fresh -DCMAKE_C_FLAGS=-mcpu=cortex-m7 -DCMAKE_BUILD_TYPE=Release
# And then 
  make
