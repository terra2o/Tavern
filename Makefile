CC := gcc

WARNINGS := -Wall -Wextra
STD      := -std=c99
INCLUDE  := -Iinclude
LIBS     := -lm -lncurses

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
	src/merchant.c \
	src/ui.c \
	src/log.c \
	src/pathway.c \
	src/save.c

OBJ := $(SRC:.c=.o)

TARGET := tavern

all: debug

debug: CFLAGS := $(STD) $(WARNINGS) $(DBG_FLAGS)
debug: SAN    := $(DBG_SAN)
debug: $(TARGET)

release: CFLAGS := $(STD) $(WARNINGS) $(REL_FLAGS)
release: SAN    := $(REL_SAN)
release: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(SAN) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(SAN) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all debug release clean
