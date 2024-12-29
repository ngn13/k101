CSRCS = $(shell find src -type f -name '*.c')
OBJS  = $(patsubst %.c,%.elf,$(CSRCS))

all: $(OBJS)

%.elf: %.c
	$(CC) -static -o $@ $^

format:
	clang-format -i -style=file $(CSRCS)

.PHONY: format
