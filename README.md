Simple ANSI terminal emulator, based on SDL2 library.

# Prerequisites

On Linux:

    sudo apt-get install libsdl2-dev libsdl2-ttf-dev fonts-dejavu

On MacOS:

    brew install sdl2 sdl2_ttf

# Build

Compile the terminal emulator from sources and install into /usr/local:

    make
    make install

Run tests:

    make test
