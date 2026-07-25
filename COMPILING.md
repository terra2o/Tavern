# COMPILING

## Modern systems (macOS, Linux, Windows, *BSD)

Compiling on modern systems is the easiest since you already have access to all the tools you need.

After downloading/installing the dependencies, simply enter `make release` into your terminal, when you're in the game's directory.

## Old systems (Windows 95/98/XP..., DOS...)

This is the trickier part! First, (although I'm planning to port the code to c89) you need a c99 compiler, and you need (if you're on Windows/DOS) PDCurses.

If you're on a virtual machine, you can make a shared directory between the host and guest. If you're on physical hardware, flash/copy the game onto some disk you can use on that computer.

### For DOS, you need:

- One of the available compilers: DJGPP, Watcom...
    - I recommend DJGPP since it would require the least amount of changes to the Makefile, since it's basically GCC retargeted for DOS.
    - Use a modern DJGPP build (GCC 12 or newer) if you can. Vintage DJGPP 2.03/2.04 installs only support C90 plus a handful of C99 features in the runtime library, even though the compiler itself may accept more. You may hit gaps in headers like `stdint.h` before Tavern's c89 port lands.
- PDCurses, built for DOS

Steps:

1. Download and install DJGPP (or Watcom).
2. Download PDCurses and build the `pdcurses/dos` target with DJGPP, producing `pdcurses.a`.
3. Point the Makefile (or your compile command) at PDCurses' `dos` headers/lib instead of system ncurses.
4. Build Tavern, then transfer the resulting `.exe` to your DOS machine/VM (via shared folder, floppy image, or null modem/serial transfer if you're really committed to the bit).
5. Run it. You'll need CWSDPMI (DJGPP's DOS extender stub) alongside the executable, unless it's statically bundled depending on how you built it.

### For Windows 95/98/XP, you need:

- MinGW (an older release, since current MinGW targets modern Windows/NT based systems; Win9x support was dropped from later toolchains) or MSVC of the era. On a modern Linux box, cross-compiling with `i686-w64-mingw32-gcc` (e.g. Fedora's `mingw32-gcc` package) works fine and still emits binaries compatible with the older Win32 API subset.
- PDCurses, built statically for the `wincon` (Windows console) platform using the same compiler. Distro PDCurses packages (e.g. Fedora's `mingw32-pdcurses`) are typically DLL-only, so `make windows` builds it from the vendored source in `vendor/pdcurses/` instead — see below.

Steps:

1. Install `i686-w64-mingw32-gcc` (or an old native MinGW build if compiling on Windows itself).
2. Run `make windows` from the project root. This will:
   - Build a static `pdcurses.a` from `vendor/pdcurses/wincon` (only the first time, or after `make clean-windows`).
   - Cross-compile Tavern against it, statically, producing `bin/tavern.exe`.
3. Copy `bin/tavern.exe` to the target machine/VM and run it from a command prompt. Nothing else needs to be bundled — it only depends on core system DLLs (`KERNEL32`, `USER32`, `ADVAPI32`, `msvcrt`), no PDCurses DLL or MinGW runtime DLLs, since everything is linked statically (`-static-libgcc`, static `pdcurses.a`).

Run `make clean-windows` to remove both Tavern's and PDCurses' build artifacts if you need a fully clean rebuild (e.g. after switching compiler versions).

### Notes

- Since Tavern currently isn't c89 clean, older/stricter compilers (real mode Turbo C, etc.) may reject the source outright. Stick to DJGPP/Watcom/MinGW for now.
- Once the c89 port lands, this section will get simpler and open the door to more vintage toolchains.
