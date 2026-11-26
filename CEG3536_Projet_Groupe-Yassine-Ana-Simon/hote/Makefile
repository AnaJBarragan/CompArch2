# CEG 3536 — Projet final — essais hôte (E11, E15 à E20, composante L5-C)
#   make            exécute tous les essais, même si l'un d'eux échoue
#   make evq        file d'événements (laboratoire 3, non-régression ; L5-A)
#   make tableau    tableau de bord (parties I et II)
#   make trajet     suivi du trajet (E16)
#   make regul      régulateur de vitesse (E17)
#   make surv       surveillances et mode essai (E18 à E20)
#   make fsm        machine à états + code généré (équipe, E9, E15 à E19)
#   make automate   vérification, génération, puis comparaison des traces du
#                   modèle et du code généré (E15, T12)
#   make traces     make automate sur les trois séquences de l'équipe (L5-B)
#   make fautif     vérification de la version fautive : contre-exemple attendu
#   make cible      régénère automate_gen.c/.h dans Core/ (après toute
#                   modification de automate.json)
#   make rejeu      rejeu des traces CSV de preuves/ (L5-C)
#   make clean
# Autre table ou autre séquence :
#   make automate AUTOMATE=automate/exemple.json SEQUENCE=automate/exemple_sequence.txt
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
AUTOMATE ?= automate/automate.json
SEQUENCE ?= automate/sequence1.txt
GEN       = automate/gen
S         = ../Core/Src
ENTETES   = $(wildcard ../Core/Inc/*.h) banc.h

all:
	-$(MAKE) --no-print-directory evq
	-$(MAKE) --no-print-directory tableau
	-$(MAKE) --no-print-directory trajet
	-$(MAKE) --no-print-directory regul
	-$(MAKE) --no-print-directory surv
	-$(MAKE) --no-print-directory fsm
	-$(MAKE) --no-print-directory traces

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
fsm: test_fsm$(X)
	./test_fsm$(X)

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
# fsm.c utilise le code généré pour la cible (Core/) : make cible d'abord
test_fsm$(X): test_fsm.c $(S)/fsm.c $(S)/automate_gen.c $(S)/essai.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_fsm.c $(S)/fsm.c $(S)/automate_gen.c $(S)/essai.c
# rejeu (L5-C) : mêmes modules sans matériel que sur la cible, données CSV
test_rejeu$(X): test_rejeu.c $(S)/rejeu.c $(S)/trajet.c $(S)/regul.c $(S)/surv.c $(ENTETES)
	$(CC) $(CFLAGS) -o $@ test_rejeu.c $(S)/rejeu.c $(S)/trajet.c $(S)/regul.c $(S)/surv.c

# Le code généré pour l'essai hôte va dans automate/gen ; pour la cible :
#   make cible
automate:
	$(OUTIL) generer $(AUTOMATE) --dossier $(GEN)
	$(CC) $(CFLAGS) -I$(GEN) -o test_automate$(X) test_automate.c $(GEN)/automate_gen.c
	$(OUTIL) simuler $(AUTOMATE) $(SEQUENCE) --sortie $(GEN)/trace_modele.txt
	./test_automate$(X) $(SEQUENCE) $(GEN)/trace_code.txt
	$(OUTIL) comparer $(GEN)/trace_modele.txt $(GEN)/trace_code.txt

traces:
	$(MAKE) --no-print-directory automate SEQUENCE=automate/sequence1.txt
	$(MAKE) --no-print-directory automate SEQUENCE=automate/sequence2.txt
	$(MAKE) --no-print-directory automate SEQUENCE=automate/sequence3.txt

fautif:
	-$(OUTIL) verifier automate/automate_fautif.json --rapport automate/rapport_fautif.txt

cible:
	$(OUTIL) verifier $(AUTOMATE) --rapport automate/rapport_verificateur.txt
	$(OUTIL) generer $(AUTOMATE) --inc ../Core/Inc --src ../Core/Src

# une trace CSV par scénario (telemetrie.py --extraire) dans preuves/ ; la
# même trace, convertie par csv_vers_c.py, est rejouée sur la cible (commande j)
REJEU ?= $(wildcard preuves/*.csv)
rejeu: test_rejeu$(X)
	for f in $(REJEU); do ./test_rejeu$(X) $$f $${f%.csv}_decisions.txt || exit 1; done

clean:
	rm -f test_evq$(X) test_tableau$(X) test_trajet$(X) test_regul$(X) test_surv$(X) test_automate$(X)
	rm -f test_fsm$(X) test_rejeu$(X)
	rm -rf $(GEN)

.PHONY: all evq tableau trajet regul surv fsm automate traces fautif cible rejeu clean
