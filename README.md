# Nintendo Entertainment System (NES) Emulator

C++ NES Implementation for My Own Learning Porpose.

# Archetecture

              +-------------+   +---------------+   +---------------+   +---------------+
              |             |   |               |   |               |   |               |
        +-----+ CPU (2AO3)  |   | MEMORY (2KB)  |   | APU           |   | CARTRIGDE     |
        |     |             |   | $0000 - $07FF |   | $4000 - $4017 |   | $4020 - $FFFF |
        |     +-------------+   +---------------+   +--------------     ++--------------+
+-------+     |                 |                   |                   ||
|       |     |                 |                   |                   ||
| CLOCK |     +-----------------+-------------------+-------------------+---------------> BUS
|       |     |                   +- - - - - - - - - - - - - - - - - - - +
+-------+     |                   |
        |     +---------------+   +----------------+   +---------------+
        |     | $2000 - $2007 |   |                |   |               |
        +-----+ PPU (2CO2)    |   | GRAPHICS (8KB) |   | VRAM (2KB)    |
              |               |   | $0000 - $1FFF  |   | $2000 - $27FF |
              +---------------+   +----------------+   +---------------+
              |                   |                    |
              |                   |                    |
              +-------------------+--------------------+--------------------------------> BUS

# References

1. [NesDev](http://nesdev.com/)

