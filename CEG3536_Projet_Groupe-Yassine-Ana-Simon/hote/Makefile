# CEG 3536 — Projet final — essais hôte (E11, E15 à E20, composante L5-C)
#   make            exécute tous les essais, même si l'un d'eux échoue
#   make evq        file d'événements (laboratoire 3, non-régression)
#   make tableau    tableau de bord (partie I)
#   make trajet     suivi du trajet (E16)
#   make regul      régulateur de vitesse (E17)
#   make surv       surveillances et mode essai (E18 à E20)
#   make automate   vérification, génération, puis comparaison des traces du
#                   modèle et du code généré (E15, T12)
#   make clean
# Automate de l'équipe :
#   make automate AUTOMATE=automate/automate.json SEQUENCE=automate/sequence1.txt
# Windows : MinGW/MSYS2 ; Linux : gcc. Les commandes gcc équivalentes figurent
# en tête de chaque fichier test_*.c pour qui n'a pas make.
# test_evq.c, evq.c et evq_test.c viennent du projet du laboratoire 3 ;
# test_tableau.c, tableau.c et tableau_test.c, du complément du laboratoire 4.
CC       ?= gcc
CFLAGS    = -std=gnu11 -Wall -Wextra -O0 -g -I../Core/Inc
WIN       = $(filter Windows_NT,$(OS))
X         = $(if $(WIN),.exe,)
PYTHON   ?= $(if $(WIN),python,python3)
OUTIL     = $(PYTHON) ../outils/automate.py
AUTOMATE ?= automate/exemple.json
SEQUENCE ?= automate/exemple_sequence.txt
GEN       = automate/gen
S         = ../Core/Src
ENTETES   = $(wildcard ../Core/Inc/*.h) banc.h

all:
	-$(MAKE) --no-print-directory evq
	-$(MAKE) --no-print-directory tableau
	-$(MAKE) --no-print-directory trajet
	-$(MAKE) --no-print-directory regul
	-$(MAKE) --no-print-directory surv
	-$(MAKE) --no-print-directory automate

evq: test_evq$(X)
	./test_evq$(X)
tableau: test_tableau$(X)
	./test_tableau$(X)
trajet: test_trajet$(X)
	./test_trajet$(X)
regul: test_regul$(X)
	./test_regul$(X)
surv: test_surv$(X)
	./test_surv$(X)

# crit_hote.c remplace crit.s sur hôte (evq.c appelle crit_entrer / crit_sortir)
test_evq$(X): test_evq.c $(S)/evq.c $(S)/evq_test.c crit_hote.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_evq.c $(S)/evq.c $(S)/evq_test.c crit_hote.c
test_tableau$(X): test_tableau.c $(S)/tableau.c $(S)/tableau_test.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_tableau.c $(S)/tableau.c $(S)/tableau_test.c
test_trajet$(X): test_trajet.c $(S)/trajet.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_trajet.c $(S)/trajet.c
test_regul$(X): test_regul.c $(S)/regul.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_regul.c $(S)/regul.c
test_surv$(X): test_surv.c $(S)/surv.c $(S)/essai.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_surv.c $(S)/surv.c $(S)/essai.c

# Le code généré pour l'essai hôte va dans automate/gen ; pour la cible :
#   python ../outils/automate.py generer automate/automate.json --inc ../Core/Inc --src ../Core/Src
automate:
	$(OUTIL) generer $(AUTOMATE) --dossier $(GEN)
	$(CC) $(CFLAGS) -I$(GEN) -o test_automate$(X) test_automate.c $(GEN)/automate_gen.c
	$(OUTIL) simuler $(AUTOMATE) $(SEQUENCE) --sortie $(GEN)/trace_modele.txt
	./test_automate$(X) $(SEQUENCE) $(GEN)/trace_code.txt
	$(OUTIL) comparer $(GEN)/trace_modele.txt $(GEN)/trace_code.txt

clean:
	rm -f test_evq$(X) test_tableau$(X) test_trajet$(X) test_regul$(X) test_surv$(X) test_automate$(X)
	rm -rf $(GEN)

.PHONY: all evq tableau trajet regul surv automate clean
