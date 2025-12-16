ifeq ($(CILKSAN),1)
CXXFLAGS += -fsanitize=cilk
LDFLAGS += -fsanitize=cilk
endif

ifeq ($(CILKSCALE),1)
# Use -fcilktool=cilkscale for OpenCilk 2.0 Cilkscale instrumentation
CXXFLAGS += -fcilktool=cilkscale -DCILKSCALE
# Cilkscale runtime is automatically linked when using -fcilktool=cilkscale
endif

