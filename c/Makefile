CC=cc
CFLAGS := -Wall -std=c11 -g -O -I .
LDFLAGS := /usr/local/lib/libtar.a # -ltar
DEPS := src/helpers.c
BIN := bin/client
SRC := src/client.c
CONSTANTS =
OUTPUT_CONSTANTS =

ifneq ($(origin PORT), undefined)
	CONSTANTS += -D PORT=$(PORT)
	OUTPUT_CONSTANTS += PORT=$(PORT)
endif

ifneq ($(origin IP), undefined)
	CONSTANTS += -D IP="$(IP)"
	OUTPUT_CONSTANTS += IP=$(IP)
endif

ifneq ($(origin CWD), undefined)
	CONSTANTS += -D CWD="$(CWD)"
	OUTPUT_CONSTANTS += CWD=$(CWD)
endif

ifneq ($(origin OUTPUT_SIZE), undefined)
	CONSTANTS += -D OUTPUT_SIZE=$(OUTPUT_SIZE)
	OUTPUT_CONSTANTS += OUTPUT_SIZE=$(OUTPUT_SIZE)
endif

ifneq ($(origin CHUNKSIZE), undefined)
	CONSTANTS += -D CHUNKSIZE=$(CHUNKSIZE)
	OUTPUT_CONSTANTS += CHUNKSIZE=$(CHUNKSIZE)
endif

all: $(DEPS) debug Makefile

local: $(SRC) Makefile
	@$(CC) $(CFLAGS) $(SRC) $(DEPS) -o bin/local -D DEBUG -D CWD="." $(LDFLAGS)
	@echo "CC $< -o bin/local ($@)$(OUTPUT_CONSTANTS)"

release: $(SRC) 
	@$(CC) $(CFLAGS) $(SRC) $(DEPS) -o $(BIN) $(CONSTANTS) $(LDFLAGS)
	@echo "CC $< -o $(BIN) ($@)$(OUTPUT_CONSTANTS)"

debug: $(SRC) 
	@$(CC) $(CFLAGS) $(SRC) $(DEPS) -o $(BIN) $(CONSTANTS) -D DEBUG $(LDFLAGS)
	@echo "CC $< -o $(BIN) ($@)$(OUTPUT_CONSTANTS)"

%.o: %.c 
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC $< -o $@"