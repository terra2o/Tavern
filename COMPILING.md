# COMPILING

## Modern systems (macOS, Linux, Windows, *BSD)

Compiling on modern systems is the easiest since you already have access to all the tools you need.

After downloading/installing the dependencies, simply enter `make release` into your terminal, when you're in the game's directory.

## Old systems (Windows 95/98/XP..., DOS...)

This is the trickier part! First, you need a c89 compiler, and you need (if you're on Windows/DOS) PDCurses.

If you're on a virtual machine, you can make a shared directory between the host and guest. If you're on physical hardware, flash/copy the game onto some disk you can use on that computer.

### For DOS (including FreeDOS), you need:

- A DJGPP cross-compiler (GCC retargeted for DOS), typically with the triplet `i586-pc-msdosdjgpp-`.
    - This generally isn't in mainstream distro package repos, so you'll need to find a prebuilt cross-toolchain package for your distro or build one yourself from the DJGPP crossgcc build scripts.
    - A vintage DJGPP 2.03/2.04 install should work fine now that Tavern builds as straight C90 - it only needs `printf`/`sscanf`-family stdio and libc, nothing from the newer feature set.
- PDCurses' `dos` platform sources, already vendored at `vendor/pdcurses/dos/` (pulled from the same PDCurses release as `vendor/pdcurses/wincon/`).

Steps:

1. Install a DJGPP cross-toolchain so `i586-pc-msdosdjgpp-gcc` (and `-ar`, `-strip`) are on your `PATH`.
2. Run `make dos` from the project root. This will:
   - Build a static `pdcurses.a` from `vendor/pdcurses/dos` (only the first time, or after `make clean-dos`).
   - Cross-compile Tavern against it, producing `bin/TAVERN.EXE`.
3. Copy `bin/TAVERN.EXE` to your FreeDOS machine/VM (shared folder, floppy/disk image, or null modem/serial transfer if you're really committed to the bit) and run it.
4. You'll need `CWSDPMI.EXE` (DJGPP's DOS extender stub) somewhere on the DOS `PATH` alongside the executable — DJGPP binaries are 32-bit protected-mode and load it at startup.

Run `make clean-dos` to remove both Tavern's and PDCurses' DOS build artifacts if you need a fully clean rebuild (e.g. after switching compiler versions).

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

- Tavern builds clean as C90/c89 now, so stricter compilers of the era (Watcom, older Turbo C/Borland C++ toolchains, etc.) should accept the source without the gaps that used to require sticking to DJGPP/MinGW specifically. If you get one of those working, let us know.
