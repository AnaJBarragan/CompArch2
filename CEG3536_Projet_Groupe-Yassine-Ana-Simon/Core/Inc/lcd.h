/* ---------------------------------------------------------------------------
 * lcd.h — afficheur 16 x 2 et rétroéclairage RVB (CEG 3536, laboratoire 4, E3)
 * Le module tient deux images de 2 x 16 caractères : celle que l'application
 * demande (lcd_ligne) et celle que l'afficheur montre. lcd_step, appelée au
 * plus une fois par tick par le répartiteur du bus, lance UNE transaction :
 * une étape de l'initialisation, une couleur, une position de curseur ou un
 * caractère. Seuls les caractères qui diffèrent sont réécrits (E3, E11).
 * ------------------------------------------------------------------------- */
#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_COLONNES    16u
#define LCD_LIGNES      2u

void lcd_init(void);                /* arme la séquence d'initialisation (aucune transaction) */
/* texte de la ligne 0 ou 1 : les 16 premiers caractères de texte16, complétés
 * par des espaces si la chaîne est plus courte. Aucune transaction.           */
void lcd_ligne(uint32_t ligne, const char *texte16);
void lcd_couleur(uint8_t r, uint8_t v, uint8_t b);   /* couleur demandée ; aucune transaction */
void lcd_step(void);                /* au plus une transaction par appel       */
bool lcd_pret(void);                /* initialisation terminée                 */
bool lcd_a_jour(void);              /* plus rien à envoyer (image et couleur)  */
uint32_t lcd_echecs(void);          /* erreurs consécutives sur le LCD (E12)   */

#endif /* LCD_H */
