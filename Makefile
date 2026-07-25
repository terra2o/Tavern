CC := gcc
CC_WIN      := i686-w64-mingw32-gcc
AR_WIN      := i686-w64-mingw32-ar
STRIP_WIN   := i686-w64-mingw32-strip
WINDRES_WIN := i686-w64-mingw32-windres

WARNINGS := -Wall -Wextra
STD      := -std=c99
INCLUDE  := -Iinclude
CURSES_LIB := -lncurses
LIBS     := -lm $(CURSES_LIB)

PDCURSES_DIR := vendor/pdcurses
PDCURSES_LIB := $(PDCURSES_DIR)/wincon/pdcurses.a
INCLUDE_WIN  := $(INCLUDE) -I$(PDCURSES_DIR)
LIBS_WIN     := $(PDCURSES_LIB) -lm -static-libgcc

DBG_FLAGS := -g -O0
DBG_SAN   := -fsanitize=address,undefined

REL_FLAGS := -O2 -DNDEBUG
REL_SAN   :=

SRC := \
	main.c \
	src/sim.c \
	src/sim_random.c \
	src/market.c \
	src/reputation.c \
	src/population.c \
	src/merchant.c \
	src/ui.c \
	src/log.c \
	src/advertisement.c \
	src/pathway.c \
	src/save.c \
	src/inflation.c \
	src/event.c

OBJ := $(SRC:.c=.o)

BIN_DIR = bin
TARGET = $(BIN_DIR)/tavern

all: debug

debug: CFLAGS := $(STD) $(WARNINGS) $(DBG_FLAGS) -DTAVERN_DEFAULT_COLORS
debug: SAN    := $(DBG_SAN)
debug: $(TARGET)

release: CFLAGS := $(STD) $(WARNINGS) $(REL_FLAGS) -DTAVERN_DEFAULT_COLORS
release: SAN    := $(REL_SAN)
release: $(TARGET)

windows: CC      := $(CC_WIN)
windows: CFLAGS  := $(STD) $(WARNINGS) $(REL_FLAGS) -DTAVERN_DEFAULT_COLORS
windows: SAN     :=
windows: INCLUDE := $(INCLUDE_WIN)
windows: TARGET  := $(BIN_DIR)/tavern.exe
windows: $(BIN_DIR)/tavern.exe

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SAN) $(OBJ) -o $@ $(LIBS)

$(BIN_DIR)/tavern.exe: $(OBJ) $(PDCURSES_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LIBS_WIN)

$(PDCURSES_LIB):
	$(MAKE) -C $(PDCURSES_DIR)/wincon \
		CC=$(CC_WIN) AR=$(AR_WIN) LINK=$(CC_WIN) \
		STRIP=$(STRIP_WIN) WINDRES=$(WINDRES_WIN) \
		pdcurses.a

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

%.o: %.c
	$(CC) $(CFLAGS) $(SAN) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) $(BIN_DIR)/tavern.exe

clean-windows: clean
	$(MAKE) -C $(PDCURSES_DIR)/wincon clean

.PHONY: all debug release windows clean clean-windows
