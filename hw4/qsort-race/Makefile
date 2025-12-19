CC := /opt/opencilk-2/bin/clang

TARGET = qsort-race

CFLAGS := -Wall -g -std=gnu11

ifeq ($(DEBUG),1)
	CFLAGS += -O0
else
	CFLAGS += -O3 -DNDEBUG
endif

# Cilksan race detector
ifeq ($(CILKSAN),1)
	CFLAGS += -fsanitize=cilk
	LDFLAGS += -fsanitize=cilk
endif

# Cilk support
CFLAGS += -fopencilk

CFLAGS += $(OTHER_CFLAGS)

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LDFLAGS)

clean::
	rm -rf $(TARGET) *.o *.s .cflags perf.data */perf.data cachegrind.out.* *.dSYM/

