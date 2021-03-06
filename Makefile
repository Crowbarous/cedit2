BIN = bin/$(LBITS)
SRC = src

EXEC = ./app

WARNINGS = \
	-Wall \
	-Wextra \
	-Wno-sign-compare \
	-Wno-unused \
	-Wno-attributes \
	-Wno-cast-function-type \
	-Wshadow

#APP-FLAGS += --opengl-debug
APP-FLAGS += --font-scale=20

CC = g++
override CFLAGS += \
	-O1 -g \
	$(WARNINGS) \
	--std=gnu++17 \
	-Isrc \
	-fmax-errors=1

FILES-CPP = $(shell find src/ -type f -name "*.cpp")
FILES-O = $(FILES-CPP:$(SRC)/%.cpp=$(BIN)/%.o)

LBITS = $(shell getconf LONG_BIT)

LIBS = -lSDL2_image -lSDL2_ttf -lSDL2_gfx

ifeq ($(OS), Windows_NT)
# Windows

CFLAGS += -DWINDOWS
LIBS += -Llib/$(LBITS)
LIBS += -lSDL2 -lopengl32 -lmingw32 -lGLU32

EXEC := $(EXEC).exe
FILES-O += $(BIN)/glew.o

else
# Linux

LIBS += -lSDL2 -lGL -lGLEW -lGLU
EXEC := $(EXEC)
CFLAGS += -DLINUX

endif

all: $(EXEC)
	@echo "$(EXEC) is up to date"

$(EXEC): $(FILES-O)
	@echo "Linking $@"
	@$(CC) $^ $(LIBS) -o $@

$(BIN)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $@"
	@$(CC) -MMD -c $(CFLAGS) $< -o $@

	@cp $(BIN)/$*.d $(BIN)/$*.P;
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	     -e '/^$$/ d' -e 's/$$/ :/' < $(BIN)/$*.d >> $(BIN)/$*.P
	@rm -f $(BIN)/$*.d
-include $(shell find $(BIN) -type f -name "*.P")

$(BIN)/glew.o: include/GL/glew.c
	@echo "Compiling $@"
	@$(CC) -c $(CFLAGS) $^ -o $@

run: all
	@echo "Running:"
	$(EXEC) $(APP-FLAGS)

# typos
ru: run
n:
rnu: run

gdb: all
	@echo "Debugging:"
	gdb $(EXEC)
valgrind: all
	valgrind $(EXEC) --leak-check=full
