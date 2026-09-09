#ifndef CAPTURA_H
#define CAPTURA_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// Evitamos incluir recursivamente si no es necesario, o lo incluimos de forma cruzada usando pre-declaraciones si hace falta.
// Ya que config_analizador.h tiene la estructura de configuración general.
struct Config_Analizador_t; // Forward declaration si es necesario, o incluimos aquí.

void Conf_Captura(void); // Asumimos que usa config_global internamente o adaptamos la firma si es necesario.
void Init_Captura();
void Stop_Captura();
bool Capture_IsFinished(void);

#endif
