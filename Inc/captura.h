#ifndef CAPTURA_H
#define CAPTURA_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// evitamos incluir recursivamente si no es necesario
// ya que config_analizador.h tiene la configuracion general
struct Config_Analizador_t; // declaracion adelantada si es necesario

void Conf_Captura(void); // asumimos que usa config_global internamente
void Inicializar_Captura();
void Detener_Captura();
bool Captura_Terminada(void);

#endif
