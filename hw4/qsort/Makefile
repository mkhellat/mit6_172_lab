CC := /opt/opencilk-2/bin/clang

TARGET = qsort

CFLAGS := -Wall -g -std=gnu11

ifeq ($(DEBUG),1)
	CFLAGS += -O0
else
	CFLAGS += -O3 -DNDEBUG
endif

# Cilkscale scalability analyzer
ifeq ($(CILKSCALE),1)
	CFLAGS += -fcilktool=cilkscale
	LDFLAGS += -fcilktool=cilkscale
endif

# Cilk support
CFLAGS += -fopencilk

CFLAGS += $(OTHER_CFLAGS)

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LDFLAGS)

clean::
	rm -rf $(TARGET) *.o *.s .cflags perf.data */perf.data cachegrind.out.* *.dSYM/

