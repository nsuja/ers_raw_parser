CC=gcc
LD=gcc
INSTALL=install


INSTALL_LIB_PATH=/usr/lib/

SRC_PATH=./src/
OUTPUT_PATH=./lib/
OUTPUT=libers_raw_parser.so

TOOL=ers_raw_parser_tool

TOOL_BIN_PATH=./bin/
TOOL_OBJECTS=tool/main.o
TOOL_LDFLAGS=-Wl,--no-as-needed -lm -lers_raw_parser

OBJECTS=src/ers_raw_parser.o
HEADERS=src/ers_raw_parser.h

CINCLUDE_PATH= -I$(SRC_PATH)
CFLAGS=-g -Wall -fPIC $(CINCLUDE_PATH)
LDFLAGS=-shared -Wl,--no-as-needed,-soname,$(OUTPUT) -lm

all: $(OUTPUT)

tool: $(TOOL)

clean:
	- $(RM) $(OUTPUT) $(OBJECTS) *~ $(SRC_PATH)/*~

install:
	- $(INSTALL) "$(OUTPUT_PATH)/$(OUTPUT)" $(INSTALL_LIB_PATH)

$(OUTPUT): $(OUTPUT_PATH) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o "$(OUTPUT_PATH)/$@"

$(TOOL): $(TOOL_BIN_PATH) $(TOOL_OBJECTS)
	$(CC) $(TOOL_LDFLAGS) $(TOOL_OBJECTS) -o "$(TOOL_BIN_PATH)/$@"

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o "$@" "$<"

$(OUTPUT_PATH):
	- mkdir $(OUTPUT_PATH)

$(TOOL_BIN_PATH):
	- mkdir $(TOOL_BIN_PATH_PATH)
