SHELL := /bin/bash
CC = gcc
#Using -Ofast instead of -O3 might result in faster code,
#but is supported only by newer GCC versions
CFLAGS = -std=gnu99 -lm -pthread -O3 -march=native -Wall -funroll-loops \
				 -Wno-unused-variable
DBGFLAGS = -lm -pthread -O0 -g3

all 	: clean dcme run

dcme 	:	vectors/dcme.c
	$(CC) vectors/dcme.c -o bin/dcme $(CFLAGS)
debug : vectors/dcme.c
	$(CC) vectors/dcme.c -o bin/dcme $(DBGFLAGS)
run		:
	@echo -en "\033[1;36m"; printf '==== RUN'; echo -e "\033[0m"
	@echo -en "\033[1;36m"; printf '=%.0s' {1..80}; echo -e "\033[0m"
	@./bin/dcme
	@echo -en "\033[1;36m"; printf '=%.0s' {1..80}; echo -e "\033[0m"
prelude :
	@echo -en "\033[1;33m"; printf '*%.0s' {1..80}; echo -e "\033[0m"
	@echo -en "\033[1;31m"; printf '==== REMOVE AND MAKE'; echo -e "\033[0m"
clean		: prelude
	rm -rf ./bin/dcme
