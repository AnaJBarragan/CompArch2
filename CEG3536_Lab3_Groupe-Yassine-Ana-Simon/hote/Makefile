# CEG 3536 — Laboratoire 3 — essais hôte de la file d'événements (jalon J2)
#   make          construit et exécute test_evq (Windows : MinGW/MSYS2 ; Linux : gcc)
#   make clean
CC      ?= gcc
CFLAGS   = -std=gnu11 -Wall -Wextra -O0 -g -I../Core/Inc
SRCS     = test_evq.c ../Core/Src/evq.c ../Core/Src/evq_test.c
EXE      = test_evq$(if $(filter Windows_NT,$(OS)),.exe,)

all: $(EXE)
	./$(EXE)

$(EXE): $(SRCS) ../Core/Inc/evq.h ../Core/Inc/evq_test.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(EXE)

.PHONY: all clean
