# UNDER CONSTRUCTION

# Compile in command line
g++ main.cpp `sdl2-config --cflags --libs`

# Compile in xcode
Add `Other Linker Flags` `-lSDL2`
Add `Library Search Paths` `/usr/local/lib`
Add `Header Search Paths` `/usr/local/include/SDL2` and `/usr/X11R6/include`


