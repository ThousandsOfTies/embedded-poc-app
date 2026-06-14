# Gapless Agent Runtime embedded PoC app

CC = aarch64-linux-gnu-gcc

.PHONY: all clean

all:
	$(MAKE) -C app CC=$(CC)

clean:
	$(MAKE) -C app clean
