# AgentCockpit embedded PoC app

CC_CROSS  = aarch64-linux-gnu-gcc
CC_NATIVE = gcc

.PHONY: cross native clean

cross:
	$(MAKE) -C app CC=$(CC_CROSS)

native:
	$(MAKE) -C app CC=$(CC_NATIVE)

clean:
	$(MAKE) -C app clean
