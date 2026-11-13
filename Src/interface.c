#include "interface.h"
#include <stdio.h>

AppState_t estado_actual = STATE_MAIN_MENU;
uint8_t necesita_refresco = 1;

// constantes de diseño
#define ROW_H 30
#define ROW_SPACE 5
#define ROW_0_Y 40
#define ROW_1_Y (ROW_0_Y + ROW_H + ROW_SPACE)
#define ROW_2_Y (ROW_1_Y + ROW_H + ROW_SPACE)

// diseño visor de ondas
#define WAVE_Y_START      18
#define WAVE_Y_END        (DISP_HEIGHT - 30)
#define WAVE_AREA_HEIGHT  (WAVE_Y_END - WAVE_Y_START)
#define GET_LANE_HEIGHT(n)       (WAVE_AREA_HEIGHT / ((n) ? (n) : 1))
#define LABEL_WIDTH       36
#define WAVE_HIGH_Y(base) ((base) + 8)
#define WAVE_LOW_Y(base, lh)  ((base) + (lh) - 10)

#define WAVE_COLOR_HIGH   COLOR_SIG_HIGH
#define WAVE_COLOR_LOW    COLOR_SIG_LOW
#define WAVE_COLOR_LABEL  COLOR_SIG_LABEL
#define WAVE_COLOR_SEP    COLOR_SIG_SEP

// tablas de busqueda

const char* cadenas_disparo[]  = {"RISING", "FALLING", "NONE"};

// capturar buffer y estado del visor
CaptureBuffer_t captura;
uint32_t desplazamiento_t_visor = 0;
uint8_t zoom_visor = 6;   // indice de divisores_zoom
uint8_t visor_necesita_redibujo_completo = 1;
uint8_t modo_zoom_encoder = 0;
int8_t indice_boton_seleccionado = -1;
bool ha_capturado = false;

extern void AlClicVolverVisor(void);
extern void AlClicTamaño(void);

// botones menu main
Button_t botones_menu_principal[5] = {
    {150, 40,  120, 35, COLOR_UI_BTN, "Freq",  AlClicFrec, 0},
    {150, 85,  120, 35, COLOR_UI_BTN, "Size",  AlClicTamaño, 0},
    {150, 130, 120, 35, COLOR_UI_BTN, "Config", AlClicConfig, 0},
    {150, 175, 120, 35, COLOR_UI_REC, "REC",   AlClicCaptura, 0},
    {0,   210, 80,  30, COLOR_UI_BTN, "Back", AlClicVolverVisor, 0},
};

// botones de ajustes
#define SETTINGS_CONF_SIZE 5
Button_t botones_ajustes[SETTINGS_CONF_SIZE] = {
    {DISP_WIDTH/3, ROW_0_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "I2C",       AlClicProtocolo, 0},
    {DISP_WIDTH/3, ROW_1_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "frecuencia", AlClicFrec,     0},
    {DISP_WIDTH/3, ROW_2_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "RISING",    AlClicDisparo, 0},
    {DISP_WIDTH/3, ROW_2_Y + ROW_H + 5, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "HEX", AlClicRadix, 0},
    {0, DISP_HEIGHT-30,     DISP_WIDTH/3,       ROW_H, COLOR_UI_BTN, "Back",      AlClicAtras,     0},
};

// botones visor
#define VIEWER_BTN_W (DISP_WIDTH / 4)
Button_t botones_visor[5] = {
    {VIEWER_BTN_W * 0, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Config",  AlClicConfig,      0},
    {VIEWER_BTN_W * 1, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "<",    AlClicDesplazarIzq,  0},
    {VIEWER_BTN_W * 2, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, ">",    AlClicDesplazarDer, 0},
    {VIEWER_BTN_W * 3, DISP_HEIGHT - 30, VIEWER_BTN_W, 30, COLOR_UI_BTN, "Back", AlClicAtras,        0},
    {0,                0,                30,           18, COLOR_UI_BTN, "S",    AlClicAlternarModo,  0},
};

// botones config protocolo
extern void AlClicUARTLsbInv(void);
Button_t botones_config_protocolo[7] = {
    {DISP_WIDTH/3, ROW_0_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "I2C",       AlClicProtocolo, 0},
    {DISP_WIDTH/3, ROW_1_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "Setting",   AlClicAjuste,  0},
    {DISP_WIDTH/3, ROW_2_Y, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "HEX",       AlClicRadix, 0},
    {DISP_WIDTH/3, ROW_2_Y + ROW_H + 5, (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "8N1",       AlClicParamsUART, 0},
    {DISP_WIDTH/3, ROW_2_Y + 2*(ROW_H + 5), (DISP_WIDTH * 2)/3, ROW_H, COLOR_UI_BTN, "LSB/Norm",  AlClicUARTLsbInv, 0},
    {0, DISP_HEIGHT-30,     DISP_WIDTH/3,       ROW_H, COLOR_UI_BTN, "Back",      AlClicAtrasConfig, 0},
    {DISP_WIDTH/3*2, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_BTN, "Decode", AlClicDecodificar,     0},
};

// botones vista tabla
Button_t botones_vista_tabla[1] = {
    {0, DISP_HEIGHT-30, DISP_WIDTH/3, ROW_H, COLOR_UI_BTN, "Back", AlClicAtrasConfig, 0},
};

View_t vistas[] = {
    [STATE_MAIN_MENU]       = {botones_menu_principal, 5, Dibujar_MenuPrincipal},
    [STATE_VIEWER]          = {botones_visor, 5, Dibujar_Visor},
    [STATE_SETTINGS]        = {botones_ajustes, SETTINGS_CONF_SIZE, Dibujar_Ajustes},
    [STATE_PROTOCOL_CONFIG] = {botones_config_protocolo, 7, Dibujar_ConfigProtocolo},
    [STATE_TABLE_VIEW]      = {botones_vista_tabla, 1, Dibujar_VistaTabla},
};

// ayudantes de ajustes
void ActualizarMenuAjustes(void) {
    botones_ajustes[0].texto = (char*)Config_ObtenerTextoProtocolo(conf.protocolo);
    botones_ajustes[1].texto = (char*)Config_ObtenerTextoFrec(conf.frecuencia);
    botones_ajustes[2].texto = (char*)cadenas_disparo[config_global.flanco];
    botones_ajustes[3].texto = (char*)Config_ObtenerTextoRadix(conf.radix);
}

void LimpiarBotones(){
    DISP_LlenarRect(0, DISP_HEIGHT-30, DISP_WIDTH, 30, COLOR_BG_DARKER);
}

void LimpiarPrincipal(){
    DISP_LlenarRect(0, 18, DISP_WIDTH, DISP_HEIGHT - 30, COLOR_BG_DARK);
}

void DibujarBarraEstado(){
    DISP_LlenarRect(0, 0, DISP_WIDTH, 18, COLOR_UI_STATUS);
    
    const char* titulo = "SETTINGS";
    switch(estado_actual) {
        case STATE_MAIN_MENU: titulo = "MAIN MENU"; break;
        case STATE_VIEWER: titulo = "SIGNAL"; break;
        case STATE_SETTINGS: titulo = "SETTINGS"; break;
        case STATE_PROTOCOL_CONFIG: titulo = "ANALYZER CONFIGURATION"; break;
        case STATE_TABLE_VIEW: titulo = "DATA TABLE"; break;
    }
    DISP_DibujarTextoCentrado(DISP_WIDTH/3, 0, DISP_WIDTH/3, 18, titulo, COLOR_UI_STATUS_T, COLOR_UI_STATUS);
}

// retrollamadas de dibujo
void Dibujar_MenuPrincipal(){
    DISP_LlenarRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    
    // etiquetas izquierda, botones derecha
    DISP_DibujarTextoCentrado(0, 47, 140, 20, "Sampling Freq", COLOR_SIG_LABEL, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, 92, 140, 20, "Buffer Size (B)", COLOR_SIG_LABEL, COLOR_BG_DARK);
    
    botones_menu_principal[0].texto = (char*)Config_ObtenerTextoFrec(conf.frecuencia);
    botones_menu_principal[1].texto = (char*)Config_ObtenerTextoTamaño(conf.tamaño_buffer);
    
    if (ha_capturado) {
        vistas[STATE_MAIN_MENU].conteoBotones = 5;
    } else {
        vistas[STATE_MAIN_MENU].conteoBotones = 4;
    }
    
    View_t vista = vistas[estado_actual];
    for (int i = 0; i < vista.conteoBotones; i++) {
        vista.botones[i].estaActivo = (i == indice_boton_seleccionado);
        UI_DibujarBoton(&vista.botones[i]);
    }
}

static void Dibujar_VisorEstatico(void) {
    DISP_LlenarRect(0, WAVE_Y_START, DISP_WIDTH, WAVE_AREA_HEIGHT, COLOR_BG_DARK);
    DibujarBarraEstado();

    const ProtocolPinConfig_t *cfg = Config_ObtenerConfigPin(conf.protocolo);
    uint16_t alto_carril = GET_LANE_HEIGHT(cfg->num_canales_activos);

    for (uint8_t ch = 0; ch < cfg->num_canales_activos; ch++) {
        uint16_t carril_y = WAVE_Y_START + ch * alto_carril;

        if (ch > 0) {
            DISP_DibujarLineaH(0, carril_y, DISP_WIDTH, WAVE_COLOR_SEP);
        }

        uint16_t etiqueta_y = carril_y + (alto_carril / 2) - 6;
        for (uint8_t i = 0; i < 4 && cfg->canales[ch].etiqueta[i] != '\0'; i++) {
            DISP_DibujarCaracter(2 + i * 8, etiqueta_y, cfg->canales[ch].etiqueta[i],
                          WAVE_COLOR_LABEL, COLOR_BG_DARK);
        }
    }
}


// arreglo de divisores de zoom. indice 0 es 1/1 (sin alejar), indices mayores alejan mas.
const float divisores_zoom[] = {
    0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f, 50000.0f, 100000.0f
};
#define MAX_ZOOM_IDX (sizeof(divisores_zoom)/sizeof(divisores_zoom[0]) - 1)

static void Dibujar_FormasOndaVisor(void) {
    extern volatile uint32_t tiempos_buffer[];
    uint32_t desplazamiento_t = desplazamiento_t_visor;
    
    uint16_t primer_visible = 0;
    for (uint16_t i = 0; i < captura.muestras_totales; i++) {
        if (tiempos_buffer[i] <= desplazamiento_t) {
            primer_visible = i;
        } else {
            break;
        }
    }

    if (zoom_visor > MAX_ZOOM_IDX) zoom_visor = MAX_ZOOM_IDX;
    float divisor_tiempo = divisores_zoom[zoom_visor];

    const ProtocolPinConfig_t *cfg = Config_ObtenerConfigPin(conf.protocolo);
    uint16_t alto_carril = GET_LANE_HEIGHT(cfg->num_canales_activos);

    // --- dibujar regla de tiempo ---
    float ticks_objetivo = 80.0f * divisor_tiempo; // apuntar a ~80 pixeles entre marcas de tiempo
    uint32_t pasos_agradables[] = {10, 20, 50, 100, 200, 500, 
                             1000, 2000, 5000, 10000, 20000, 50000, 
                             100000, 200000, 500000, 1000000, 2000000, 5000000, 10000000};
    uint32_t paso_cuadricula = pasos_agradables[0];
    for (int i = 0; i < sizeof(pasos_agradables)/sizeof(pasos_agradables[0]); i++) {
        if (pasos_agradables[i] >= ticks_objetivo) {
            paso_cuadricula = pasos_agradables[i];
            break;
        }
    }

    uint32_t primer_t_cuadricula = ((desplazamiento_t + paso_cuadricula - 1) / paso_cuadricula) * paso_cuadricula;
    for (uint32_t t = primer_t_cuadricula; ; t += paso_cuadricula) {
        int32_t px = LABEL_WIDTH + (int32_t)((t - desplazamiento_t) / divisor_tiempo);
        if (px >= DISP_WIDTH) break;
        
        for (uint16_t yy = WAVE_Y_START; yy < WAVE_Y_END; yy += 4) {
            DISP_DibujarLineaV(px, yy, 2, COLOR_SIG_SEP);
        }
        
        char ts_str[16];
        if (paso_cuadricula >= 1000000) {
            uint32_t s = t / 1000000;
            uint32_t ds = (t % 1000000) / 100000;
            if (ds > 0) snprintf(ts_str, sizeof(ts_str), "%u.%us", (unsigned int)s, (unsigned int)ds);
            else snprintf(ts_str, sizeof(ts_str), "%us", (unsigned int)s);
        } else if (paso_cuadricula >= 1000) {
            uint32_t ms = t / 1000;
            uint32_t dms = (t % 1000) / 100;
            if (dms > 0) snprintf(ts_str, sizeof(ts_str), "%u.%ums", (unsigned int)ms, (unsigned int)dms);
            else snprintf(ts_str, sizeof(ts_str), "%ums", (unsigned int)ms);
        } else {
            snprintf(ts_str, sizeof(ts_str), "%uus", (unsigned int)t);
        }
        
        DISP_DibujarCadena(px + 2, WAVE_Y_END - 10, ts_str, COLOR_SIG_LABEL, COLOR_BG_DARK);
    }

    // --- dibujar señales de formas de onda ---
    for (uint8_t ch = 0; ch < cfg->num_canales_activos; ch++) {
        uint16_t carril_y = WAVE_Y_START + ch * alto_carril;
        uint16_t y_alto = WAVE_HIGH_Y(carril_y);
        uint16_t y_bajo  = WAVE_LOW_Y(carril_y, alto_carril);

        if (cfg->num_canales_activos == 1) {
            uint16_t amp_orig = y_bajo - y_alto;
            uint16_t y_medio = y_alto + amp_orig / 2;
            y_alto = y_medio - amp_orig / 4;
            y_bajo  = y_medio + amp_orig / 4;
        }

        uint8_t  estado_ejecucion = captura.muestras[ch][primer_visible];
        uint16_t ejecucion_x0    = LABEL_WIDTH;
        uint16_t ejecucion_y     = estado_ejecucion ? y_alto : y_bajo;

        for (uint16_t s = primer_visible + 1; s < captura.muestras_totales; s++) {
            // usando int32_t para diferencia
            int32_t diferencia_t = (int32_t)(tiempos_buffer[s] - desplazamiento_t);
            if (diferencia_t < 0) diferencia_t = 0; // alternativa si es negativo
            
            uint32_t px32 = LABEL_WIDTH + (diferencia_t) / divisor_tiempo;
            uint16_t pixel_x = (px32 > DISP_WIDTH) ? DISP_WIDTH : (uint16_t)px32;

            uint8_t estado_actual = captura.muestras[ch][s];

            if (estado_actual != estado_ejecucion || pixel_x >= DISP_WIDTH) {
                // dibujar tramo horizontal
                uint16_t longitud_ejecucion = pixel_x - ejecucion_x0;
                if (longitud_ejecucion > 0) {
                    DISP_DibujarLineaH(ejecucion_x0, ejecucion_y, longitud_ejecucion, estado_ejecucion ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
                }

                // dibujar transicion vertical
                if (pixel_x < DISP_WIDTH && estado_actual != estado_ejecucion) {
                    uint16_t nueva_y = estado_actual ? y_alto : y_bajo;
                    uint16_t arriba   = (nueva_y < ejecucion_y) ? nueva_y : ejecucion_y;
                    uint16_t h     = (nueva_y > ejecucion_y) ? (nueva_y - ejecucion_y + 1) : (ejecucion_y - nueva_y + 1);
                    DISP_DibujarLineaV(pixel_x, arriba, h, estado_actual ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
                }

                estado_ejecucion = estado_actual;
                ejecucion_x0    = pixel_x;
                ejecucion_y     = estado_actual ? y_alto : y_bajo;
            }

            if (pixel_x >= DISP_WIDTH) break;
        }

        // limpiar restante si no llegamos al borde de pantalla
        if (ejecucion_x0 < DISP_WIDTH) {
            DISP_DibujarLineaH(ejecucion_x0, ejecucion_y, DISP_WIDTH - ejecucion_x0, estado_ejecucion ? WAVE_COLOR_HIGH : WAVE_COLOR_LOW);
        }
    }

    // --- dibujar decodificador ---
    extern volatile uint32_t indice_decodificado;
    extern volatile uint8_t  buffer_decodificado[];
    extern volatile uint32_t tiempos_inicio_decodificados[];
    extern volatile uint32_t tiempos_fin_decodificados[];

    // calcular ancho de bit en ticks
    // tim2 psc=83 -> timer_freq_hz = 1 mhz
    // esto debe coincidir con formula en uart.c: timer_freq_hz / tasa_baudios
    uint32_t baudios = 0;
    if (conf.protocolo == UART) {
        baudios = config_global.protocolo.ajustes.uart.tasa_baudios;
    }

    for (uint32_t i = 0; i < indice_decodificado; i++) {
        uint32_t inicio_t = tiempos_inicio_decodificados[i];
        uint32_t fin_t   = tiempos_fin_decodificados[i];

        // calcular posiciones de pixeles
        int32_t ds = (int32_t)(inicio_t - desplazamiento_t);
        int32_t de = (int32_t)(fin_t   - desplazamiento_t);

        if (conf.protocolo == SPI) {
            uint8_t tamaño_datos = config_global.protocolo.ajustes.spi.tamaño_datos;
            if (tamaño_datos > 1 && fin_t > inicio_t) {
                float dt_bit = (float)(fin_t - inicio_t) / (tamaño_datos - 1);
                ds -= (int32_t)(dt_bit / 2.0f);
                de += (int32_t)(dt_bit / 2.0f);
            }
        }

        // omitir si esta antes de la vista
        if (de < 0) continue;

        uint16_t px_inicio = (ds < 0) ? LABEL_WIDTH
                          : (uint16_t)((uint32_t)ds / divisor_tiempo + LABEL_WIDTH);
        uint16_t px_fin   = (uint16_t)((uint32_t)(de > 0 ? de : 0) / divisor_tiempo + LABEL_WIDTH);

        if (px_inicio >= DISP_WIDTH) continue;   // totalmente fuera de la vista
        if (px_fin   >  DISP_WIDTH) px_fin = DISP_WIDTH;

        // determinar carril activo para cuadricula
        uint16_t carril_arriba = WAVE_Y_START;
        uint16_t carril_abajo;
        
        if (conf.protocolo == SPI) {
            carril_abajo = WAVE_Y_START + cfg->num_canales_activos * alto_carril;
        } else {
            carril_abajo = carril_arriba + alto_carril;
        }

        // 1. cuadricula de ancho de bits
        if (conf.protocolo == UART && baudios > 0) {
            uint32_t ticks_por_bit = TIMER_FREQ_HZ / baudios;
            if ((float)ticks_por_bit / divisor_tiempo > 3.0f) {
                uint8_t bits_paridad = (config_global.protocolo.ajustes.uart.paridad == UART_PARITY_SIN) ? 0 : 1;
                uint8_t bits_parada = (config_global.protocolo.ajustes.uart.bits_parada == UART_STOPBITS_2) ? 2 : 1;
                uint8_t bits_totales = 1 + config_global.protocolo.ajustes.uart.bits_datos + bits_paridad + bits_parada;
                
                for (uint8_t bit = 1; bit < bits_totales; bit++) { // empezar en 1, no dibujar sobre lineas rojas de inicio/fin
                    int32_t dt_bit = (int32_t)((inicio_t + (uint32_t)(((uint64_t)bit * TIMER_FREQ_HZ) / baudios)) - desplazamiento_t);
                    if (dt_bit < 0) continue;
                    uint16_t px_bit = (uint16_t)((uint32_t)dt_bit / divisor_tiempo + LABEL_WIDTH);
                    if (px_bit >= DISP_WIDTH) break;
                    // dibujar un pixel si y otro no para efecto punteado
                    for (uint16_t yy = carril_arriba; yy < carril_abajo; yy += 2) {
                        DISP_DibujarLineaV(px_bit, yy, 1, 0x632C);
                    }
                }
            }
        } else if (conf.protocolo == SPI) {
            uint8_t tamaño_datos = config_global.protocolo.ajustes.spi.tamaño_datos;
            if (tamaño_datos > 1 && fin_t > inicio_t) {
                float dt_bit = (float)(fin_t - inicio_t) / (tamaño_datos - 1);
                if (dt_bit / divisor_tiempo > 3.0f) {
                    for (uint8_t bit = 1; bit < tamaño_datos; bit++) {
                        int32_t dt_cuadricula = (int32_t)(inicio_t + bit * dt_bit - dt_bit / 2.0f - desplazamiento_t);
                        if (dt_cuadricula < 0) continue;
                        uint16_t px_bit = (uint16_t)((float)dt_cuadricula / divisor_tiempo + LABEL_WIDTH);
                        if (px_bit >= DISP_WIDTH) break;
                        // dibujar un pixel si y otro no para efecto punteado
                        for (uint16_t yy = carril_arriba; yy < carril_abajo; yy += 2) {
                            DISP_DibujarLineaV(px_bit, yy, 1, 0x632C);
                        }
                    }
                }
            }
        }

        // 2. linea vertical roja punteada inicio y fin paquete
        for (uint16_t yy = carril_arriba; yy < carril_abajo; yy += 4) {
            if (px_inicio < DISP_WIDTH) {
                DISP_DibujarLineaV(px_inicio, yy, 2, COLOR_UI_REC);
            }
            if (px_fin < DISP_WIDTH) {
                // no dibujar linea final si hay otro paquete despues
                uint8_t fin_dibujo = 1;
                if (i + 1 < indice_decodificado && tiempos_inicio_decodificados[i+1] == fin_t) {
                    fin_dibujo = 0;
                }
                if (fin_dibujo) DISP_DibujarLineaV(px_fin, yy, 2, COLOR_UI_REC);
            }
        }

        // 3. etiqueta hex dibujada arriba de señal
        uint16_t ancho_etiqueta = px_fin - px_inicio;
        if (ancho_etiqueta > 10) {
            char cadenaValor[5];
            uint8_t modo_radix = conf.radix;
            if (modo_radix == RADIX_HEX)
                snprintf(cadenaValor, sizeof(cadenaValor), "%02X", buffer_decodificado[i]);
            else if (modo_radix == RADIX_DEC)
                snprintf(cadenaValor, sizeof(cadenaValor), "%u",  buffer_decodificado[i]);
            else {
                uint8_t c = buffer_decodificado[i];
                if (c >= 32 && c <= 126)
                    snprintf(cadenaValor, sizeof(cadenaValor), "'%c'", c);
                else
                    snprintf(cadenaValor, sizeof(cadenaValor), "%02X", c);
            }
            // dibujar centrado en el paquete en la zona superior de ondas
            uint16_t etiqueta_y = WAVE_Y_START + 2;
            DISP_DibujarCadena(px_inicio + 2, etiqueta_y, cadenaValor, COLOR_UI_ACCENT, COLOR_BG_DARK);
        }
    }
}



void Dibujar_Visor(){
    uint8_t copia_redibujo_completo = visor_necesita_redibujo_completo;
    if (visor_necesita_redibujo_completo) {
        DISP_LlenarRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
        Dibujar_VisorEstatico();
        visor_necesita_redibujo_completo = 0;
    } else {
        // limpiar solo area de ondas
        DISP_LlenarRect(LABEL_WIDTH, WAVE_Y_START, DISP_WIDTH - LABEL_WIDTH, WAVE_AREA_HEIGHT, COLOR_BG_DARK);
    }

    Dibujar_FormasOndaVisor();

    // actualizar boton modo zoom/scroll
    botones_visor[4].texto = modo_zoom_encoder ? "Z" : "S";

    // solo dibujar botones de nuevo si hubo redibujo completo
    // excepto boton de cambio de modo cuyo texto pudo cambiar
    static uint8_t ultimo_modo_zoom_encoder = 0xFF;
    if (copia_redibujo_completo) {
        View_t vista = vistas[estado_actual];
        for (int i = 0; i < vista.conteoBotones; i++) {
            vista.botones[i].estaActivo = (i == indice_boton_seleccionado);
        UI_DibujarBoton(&vista.botones[i]);
        }
    } else if (ultimo_modo_zoom_encoder != modo_zoom_encoder) {
        UI_DibujarBoton(&botones_visor[4]);
    }
    ultimo_modo_zoom_encoder = modo_zoom_encoder;
}

void Dibujar_Ajustes(){
    LimpiarPrincipal();
    DISP_LlenarRect(0, 40, DISP_WIDTH, 30, COLOR_UI_ACCENT);

    DISP_DibujarTextoCentrado(0, ROW_0_Y, DISP_WIDTH/3, ROW_H, "Protocol",  COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DibujarTextoCentrado(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Frequency", COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DibujarTextoCentrado(0, ROW_2_Y, DISP_WIDTH/3, ROW_H, "Trigger",   COLOR_BG_DARK, COLOR_UI_ACCENT);
    DISP_DibujarTextoCentrado(0, ROW_2_Y + ROW_H + 5, DISP_WIDTH/3, ROW_H, "Radix",     COLOR_BG_DARK, COLOR_UI_ACCENT);

    DibujarBarraEstado();
    LimpiarBotones();

    ActualizarMenuAjustes();

    View_t vista = vistas[estado_actual];
    for (int i = 0; i < vista.conteoBotones; i++) {
        vista.botones[i].estaActivo = (i == indice_boton_seleccionado);
        UI_DibujarBoton(&vista.botones[i]);
    }
}

void Dibujar_ConfigProtocolo(){
    DISP_LlenarRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    DISP_LlenarRect(0, 40, DISP_WIDTH, 30, COLOR_UI_ACCENT);

    DISP_DibujarTextoCentrado(0, ROW_0_Y, DISP_WIDTH/3, ROW_H, "Protocol",  COLOR_BG_DARK, COLOR_UI_ACCENT);
    
    // actualizar textos de botones segun configuracion
    botones_config_protocolo[0].texto = (char*)Config_ObtenerTextoProtocolo(conf.protocolo);
    
    if (conf.protocolo == UART) {
        DISP_DibujarTextoCentrado(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Baudrate", COLOR_SIG_LABEL, COLOR_BG_DARK);
        botones_config_protocolo[1].texto = config_global.protocolo.ajustes.uart.tasa_baudios == 9600 ? "9600" : "115200";
    } else if (conf.protocolo == SPI) {
        DISP_DibujarTextoCentrado(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Mode", COLOR_SIG_LABEL, COLOR_BG_DARK);
        static char spi_mode_str[16];
        snprintf(spi_mode_str, sizeof(spi_mode_str), "Mode %d", config_global.protocolo.ajustes.spi.modo);
        botones_config_protocolo[1].texto = spi_mode_str;
    } else {
        DISP_DibujarTextoCentrado(0, ROW_1_Y, DISP_WIDTH/3, ROW_H, "Setting", COLOR_SIG_LABEL, COLOR_BG_DARK);
        botones_config_protocolo[1].texto = "N/A";
    }

    DISP_DibujarTextoCentrado(0, ROW_2_Y, DISP_WIDTH/3, ROW_H, "Radix", COLOR_SIG_LABEL, COLOR_BG_DARK);
    botones_config_protocolo[2].texto = (char*)Config_ObtenerTextoRadix(conf.radix);
    
    if (conf.protocolo == UART) {
        vistas[STATE_PROTOCOL_CONFIG].conteoBotones = 7;
        DISP_DibujarTextoCentrado(0, ROW_2_Y + ROW_H + 5, DISP_WIDTH/3, ROW_H, "Format", COLOR_SIG_LABEL, COLOR_BG_DARK);
        
        static char uart_fmt_str[8];
        char p = config_global.protocolo.ajustes.uart.paridad == UART_PARITY_SIN ? 'N' : (config_global.protocolo.ajustes.uart.paridad == UART_PARITY_PAR ? 'E' : 'O');
        char s = config_global.protocolo.ajustes.uart.bits_parada == UART_STOPBITS_2 ? '2' : '1';
        snprintf(uart_fmt_str, sizeof(uart_fmt_str), "%d%c%c", config_global.protocolo.ajustes.uart.bits_datos, p, s);
        botones_config_protocolo[3].texto = uart_fmt_str;
        
        DISP_DibujarTextoCentrado(0, ROW_2_Y + 2*(ROW_H + 5), DISP_WIDTH/3, ROW_H, "LSB/Inv", COLOR_SIG_LABEL, COLOR_BG_DARK);
        
        static char uart_lsb_str[16];
        snprintf(uart_lsb_str, sizeof(uart_lsb_str), "%s %s", config_global.protocolo.ajustes.uart.lsb_primero ? "LSB" : "MSB", config_global.protocolo.ajustes.uart.invertido ? "INV" : "NRM");
        botones_config_protocolo[4].texto = uart_lsb_str;
    } else {
        vistas[STATE_PROTOCOL_CONFIG].conteoBotones = 7;
        botones_config_protocolo[3].texto = "";
        botones_config_protocolo[4].texto = "";
    }

    DibujarBarraEstado();

    View_t vista = vistas[estado_actual];
    for (int i = 0; i < vista.conteoBotones; i++) {
        if (vista.botones[i].texto[0] != '\0') {
            vista.botones[i].estaActivo = (i == indice_boton_seleccionado);
        UI_DibujarBoton(&vista.botones[i]);
        }
    }
}

extern volatile uint8_t buffer_decodificado[];
extern volatile uint32_t indice_decodificado;

int16_t desplazamiento_tabla = 0;   // indice de fila de primera entrada decodificada visible

void Dibujar_VistaTabla(){
    extern volatile uint32_t tiempos_inicio_decodificados[];
    LimpiarPrincipal();
    DibujarBarraEstado();
    LimpiarBotones();

    #define TABLE_ROW_H   14
    #define TABLE_FIRST_Y 38
    #define TABLE_ROWS_VISIBLE  ((DISP_HEIGHT - 40 - TABLE_FIRST_Y) / TABLE_ROW_H)

    if (indice_decodificado == 0) {
        DISP_DibujarTextoCentrado(0, 50, DISP_WIDTH, 20, "No datos decoded yet.", COLOR_SIG_LABEL, COLOR_BG_DARK);
    } else {
        // limitar desplazamiento
        int16_t desplazamiento_max = (int16_t)indice_decodificado - TABLE_ROWS_VISIBLE;
        if (desplazamiento_max < 0) desplazamiento_max = 0;
        if (desplazamiento_tabla > desplazamiento_max) desplazamiento_tabla = desplazamiento_max;
        if (desplazamiento_tabla < 0) desplazamiento_tabla = 0;

        char buffer[64];
        uint16_t fila_y = TABLE_FIRST_Y;
        
        // dibujar encabezado
        DISP_DibujarCadena(5, fila_y, "#    Time(t)      HEX  ASCII", COLOR_UI_ACCENT, COLOR_BG_DARK);
        fila_y += TABLE_ROW_H;

        for (uint32_t i = (uint32_t)desplazamiento_tabla; i < indice_decodificado; i++) {
            if (fila_y + TABLE_ROW_H > DISP_HEIGHT - 30) break;

            char ascii_c = (buffer_decodificado[i] >= 32 && buffer_decodificado[i] <= 126) ? buffer_decodificado[i] : '.';
            snprintf(buffer, sizeof(buffer), "%-4lu %-12lu 0x%02X  %c", (unsigned long)i,
                     tiempos_inicio_decodificados[i], buffer_decodificado[i], ascii_c);
            
            DISP_DibujarCadena(5, fila_y, buffer, COLOR_SIG_LABEL, COLOR_BG_DARK);
            fila_y += TABLE_ROW_H;
        }

        // barra de desplazamiento
        if (indice_decodificado > (uint32_t)TABLE_ROWS_VISIBLE) {
            uint16_t alto_barra = (DISP_HEIGHT - 40 - TABLE_FIRST_Y) * TABLE_ROWS_VISIBLE / indice_decodificado;
            if (alto_barra < 5) alto_barra = 5;
            uint16_t barra_y = TABLE_FIRST_Y + (DISP_HEIGHT - 40 - TABLE_FIRST_Y - alto_barra) * desplazamiento_tabla / desplazamiento_max;
            DISP_DibujarLineaV(DISP_WIDTH - 2, barra_y, alto_barra, COLOR_UI_ACCENT);
        }
    }
    
    View_t vista = vistas[estado_actual];
    for (int i = 0; i < vista.conteoBotones; i++) {
        vista.botones[i].estaActivo = (i == indice_boton_seleccionado);
        UI_DibujarBoton(&vista.botones[i]);
    }
}




extern volatile bool bandera_inicio_captura;

void AlClicCaptura(void) {
    bandera_inicio_captura = true;
    DISP_LlenarRect(0, 0, DISP_WIDTH, DISP_HEIGHT, COLOR_BG_DARK);
    DISP_DibujarTextoCentrado(0, DISP_HEIGHT/2 - 10, DISP_WIDTH, 20, "Capturing...", COLOR_UI_ACCENT, COLOR_BG_DARK);
}

void AlClicAtras(void) {
    estado_actual = STATE_MAIN_MENU;
    necesita_refresco = 1;
}

void AlClicVolverVisor(void) {
    estado_actual = STATE_VIEWER;
    visor_necesita_redibujo_completo = 1;
    necesita_refresco = 1;
}


AppState_t estado_config_prev = STATE_MAIN_MENU;

void AlClicConfig(void) {
    estado_config_prev = estado_actual;
    estado_actual = STATE_PROTOCOL_CONFIG;
    necesita_refresco = 1;
}


void AlClicAtrasConfig(void) {
    estado_actual = estado_config_prev;
    if (estado_actual == STATE_VIEWER) {
        visor_necesita_redibujo_completo = 1;
    }
    necesita_refresco = 1;
}

extern volatile uint32_t indice_decodificado;
extern volatile uint16_t estados_buffer[];

void EnfocarEnPaquete(int32_t indice_paquete);

void AlClicDecodificar(void) {
    indice_decodificado = 0; // reiniciar decodificaciones
    
    config_global.protocolo.protocolo_activo = conf.protocolo; // sincronizar protocolo
    
    extern void Configurar_Disparo_Protocolo(void);
    Configurar_Disparo_Protocolo(); // actualizar asignacion de pines para nuevo protocolo
    
    switch (conf.protocolo) {
        case UART:
            UART_FSM((uint16_t*)estados_buffer, captura.muestras_totales, &config_global.protocolo.ajustes.uart);
            break;
        case SPI:
            SPI_FSM((uint16_t*)estados_buffer, captura.muestras_totales, &config_global.protocolo.ajustes.spi);
            break;
        case I2C:
            I2C_FSM((uint16_t*)estados_buffer, captura.muestras_totales, &config_global.protocolo.ajustes.i2c);
            break;
        default: break;
    }
    
    if (indice_decodificado > 0) {
        EnfocarEnPaquete(0);
    }
    
    estado_actual = STATE_VIEWER;
    visor_necesita_redibujo_completo = 1;
    necesita_refresco = 1;
}

void AlClicAjuste(void) {
    if (conf.protocolo == UART) {
        if (config_global.protocolo.ajustes.uart.tasa_baudios == 9600) {
            config_global.protocolo.ajustes.uart.tasa_baudios = 115200;
        } else {
            config_global.protocolo.ajustes.uart.tasa_baudios = 9600;
        }
    } else if (conf.protocolo == SPI) {
        config_global.protocolo.ajustes.spi.modo = (config_global.protocolo.ajustes.spi.modo + 1) % 4;
    }
    necesita_refresco = 1;
}

void AlClicTabla(void) {
    if(estado_actual == STATE_TABLE_VIEW) {
        estado_actual = STATE_VIEWER;
        visor_necesita_redibujo_completo = 1;
    } else {
        estado_actual = STATE_TABLE_VIEW;
        desplazamiento_tabla = 0;  // empezar en primera entrada
    }
    necesita_refresco = 1;
}

volatile uint8_t refrescar_boton_zoom = 0;

void AlClicAlternarModo(void) {
    modo_zoom_encoder = !modo_zoom_encoder;
    necesita_refresco = 1;
}

void EnfocarEnPaquete(int32_t indice_paquete) {
    extern volatile uint32_t tiempos_inicio_decodificados[];
    extern volatile uint32_t tiempos_fin_decodificados[];

    if (indice_paquete < 0 || indice_paquete >= indice_decodificado) return;

    uint32_t inicio_t = tiempos_inicio_decodificados[indice_paquete];
    uint32_t fin_t = tiempos_fin_decodificados[indice_paquete];
    uint32_t dt = fin_t - inicio_t;

    // queremos que el paquete ocupe unos 250 pixeles
    float divisor_objetivo = (float)dt / 250.0f;
    if (divisor_objetivo <= 0.0f) divisor_objetivo = 0.1f;

    uint8_t mejor_zoom = MAX_ZOOM_IDX;
    for (uint8_t i = 0; i <= MAX_ZOOM_IDX; i++) {
        if (divisores_zoom[i] >= divisor_objetivo) {
            mejor_zoom = i;
            break;
        }
    }
    
    zoom_visor = mejor_zoom;

    // dejar pequeño margen a la izquierda
    uint32_t tiempo_margen = (uint32_t)(20.0f * divisores_zoom[mejor_zoom]);
    uint32_t desplazamiento_t_objetivo = (inicio_t > tiempo_margen) ? (inicio_t - tiempo_margen) : 0;
    
    desplazamiento_t_visor = desplazamiento_t_objetivo;
    visor_necesita_redibujo_completo = 1;
    necesita_refresco = 1;
}

int32_t ObtenerIndicePaqueteActual(void) {
    extern volatile uint32_t tiempos_buffer[];
    extern volatile uint32_t indice_decodificado;
    extern volatile uint32_t tiempos_inicio_decodificados[];
    extern volatile uint32_t tiempos_fin_decodificados[];

    if (indice_decodificado == 0) return -1;
    
    uint32_t centro_t = desplazamiento_t_visor + (uint32_t)(((DISP_WIDTH - LABEL_WIDTH) / 2.0f) * divisores_zoom[zoom_visor]);
    
    int32_t mejor_indice = 0;
    uint32_t dist_minima = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < indice_decodificado; i++) {
        uint32_t medio = tiempos_inicio_decodificados[i] + (tiempos_fin_decodificados[i] - tiempos_inicio_decodificados[i])/2;
        uint32_t distancia = (centro_t > medio) ? (centro_t - medio) : (medio - centro_t);
        if (distancia < dist_minima) {
            dist_minima = distancia;
            mejor_indice = i;
        }
    }
    return mejor_indice;
}

void AlClicDesplazarIzq(void) {
    uint32_t paso = (uint32_t)(((DISP_WIDTH - LABEL_WIDTH) * divisores_zoom[zoom_visor]) * 0.1f);
    if (paso == 0) paso = 1;
    if (desplazamiento_t_visor >= paso) {
        desplazamiento_t_visor -= paso;
    } else {
        desplazamiento_t_visor = 0;
    }
    necesita_refresco = 1;
}

void AlClicDesplazarDer(void) {
    uint32_t paso = (uint32_t)(((DISP_WIDTH - LABEL_WIDTH) * divisores_zoom[zoom_visor]) * 0.1f);
    if (paso == 0) paso = 1;
    desplazamiento_t_visor += paso;
    necesita_refresco = 1;
}

void AlDesplazarEncoderIzq(void) {
    if (estado_actual == STATE_MAIN_MENU || estado_actual == STATE_PROTOCOL_CONFIG || estado_actual == STATE_SETTINGS) {
        View_t vista = vistas[estado_actual];
        do {
            if (indice_boton_seleccionado <= 0) {
                indice_boton_seleccionado = vista.conteoBotones - 1;
            } else {
                indice_boton_seleccionado--;
            }
        } while (vista.botones[indice_boton_seleccionado].texto[0] == '\0');
        necesita_refresco = 1;
        return;
    }
    if (estado_actual == STATE_TABLE_VIEW) {
        if (desplazamiento_tabla > 0) {
            desplazamiento_tabla--;
            necesita_refresco = 1;
        }
        return;
    }
    if (estado_actual == STATE_VIEWER) {
        if (modo_zoom_encoder) {
            if (zoom_visor > 0) {
                zoom_visor--;
                necesita_refresco = 1;
            }
        } else {
            AlClicDesplazarIzq();
        }
    }
}

void AlDesplazarEncoderDer(void) {
    if (estado_actual == STATE_MAIN_MENU || estado_actual == STATE_PROTOCOL_CONFIG || estado_actual == STATE_SETTINGS) {
        View_t vista = vistas[estado_actual];
        do {
            if (indice_boton_seleccionado < 0) {
                indice_boton_seleccionado = 0;
            } else {
                indice_boton_seleccionado = (indice_boton_seleccionado + 1) % vista.conteoBotones;
            }
        } while (vista.botones[indice_boton_seleccionado].texto[0] == '\0');
        necesita_refresco = 1;
        return;
    }
    if (estado_actual == STATE_TABLE_VIEW) {
        extern volatile uint32_t indice_decodificado;
        if (desplazamiento_tabla < (int16_t)indice_decodificado - TABLE_ROWS_VISIBLE) {
            desplazamiento_tabla++;
            necesita_refresco = 1;
        }
        return;
    }
    if (estado_actual == STATE_VIEWER) {
        if (modo_zoom_encoder) {
            if (zoom_visor < MAX_ZOOM_IDX) {
                zoom_visor++;
                necesita_refresco = 1;
            }
        } else {
            AlClicDesplazarDer();
        }
    }
}

void AlPresionarLargoBotonEncoder(void) {
    if (estado_actual == STATE_VIEWER || estado_actual == STATE_TABLE_VIEW) {
        AlClicTabla();
    }
}

void AlPresionarBotonEncoder(void) {
    if (estado_actual == STATE_VIEWER) {
        AlClicAlternarModo();
    } else if (estado_actual == STATE_MAIN_MENU || estado_actual == STATE_PROTOCOL_CONFIG || estado_actual == STATE_SETTINGS) {
        View_t vista = vistas[estado_actual];
        if (indice_boton_seleccionado >= 0 && indice_boton_seleccionado < vista.conteoBotones) {
            if (vista.botones[indice_boton_seleccionado].alClic) {
                vista.botones[indice_boton_seleccionado].alClic();
            }
        }
    }
}



// retrollamadas de ajustes
void AlClicProtocolo(void) {
    conf.protocolo = (conf.protocolo + 1) % PROTOCOL_COUNT;
    necesita_refresco = 1;
}

void AlClicFrec(void){
    conf.frecuencia = (conf.frecuencia + 1) % FREQUENCY_COUNT;
    necesita_refresco = 1;
}

void AlClicTamaño(void){
    conf.tamaño_buffer = (conf.tamaño_buffer + 1) % 4;
    necesita_refresco = 1;
}

void AlClicDisparo(void){
    config_global.flanco = (config_global.flanco + 1) % 3;
    necesita_refresco = 1;
}

void AlClicRadix(void){
    conf.radix = (conf.radix + 1) % 3;
    necesita_refresco = 1;
}


void AlClicParamsUART(void){
    if (config_global.protocolo.ajustes.uart.bits_datos == 8 && config_global.protocolo.ajustes.uart.paridad == UART_PARITY_SIN && config_global.protocolo.ajustes.uart.bits_parada == UART_STOPBITS_1) {
        config_global.protocolo.ajustes.uart.paridad = UART_PARITY_PAR; // 8e1
    } else if (config_global.protocolo.ajustes.uart.bits_datos == 8 && config_global.protocolo.ajustes.uart.paridad == UART_PARITY_PAR) {
        config_global.protocolo.ajustes.uart.paridad = UART_PARITY_IMPAR; // 8o1
    } else if (config_global.protocolo.ajustes.uart.bits_datos == 8 && config_global.protocolo.ajustes.uart.paridad == UART_PARITY_IMPAR) {
        config_global.protocolo.ajustes.uart.paridad = UART_PARITY_SIN;
        config_global.protocolo.ajustes.uart.bits_parada = UART_STOPBITS_2; // 8n2
    } else if (config_global.protocolo.ajustes.uart.bits_datos == 8 && config_global.protocolo.ajustes.uart.bits_parada == UART_STOPBITS_2) {
        config_global.protocolo.ajustes.uart.bits_parada = UART_STOPBITS_1;
        config_global.protocolo.ajustes.uart.bits_datos = 7; // 7n1
    } else if (config_global.protocolo.ajustes.uart.bits_datos == 7) {
        config_global.protocolo.ajustes.uart.bits_datos = 9; // 9n1
    } else {
        config_global.protocolo.ajustes.uart.bits_datos = 8; // 8n1
        config_global.protocolo.ajustes.uart.paridad = UART_PARITY_SIN;
        config_global.protocolo.ajustes.uart.bits_parada = UART_STOPBITS_1;
    }
    visor_necesita_redibujo_completo = 1;
    necesita_refresco = 1;
}

void AlClicUARTLsbInv(void){
    if (config_global.protocolo.ajustes.uart.lsb_primero == 1 && config_global.protocolo.ajustes.uart.invertido == 0) {
        config_global.protocolo.ajustes.uart.lsb_primero = 0; // msb, nrm
    } else if (config_global.protocolo.ajustes.uart.lsb_primero == 0 && config_global.protocolo.ajustes.uart.invertido == 0) {
        config_global.protocolo.ajustes.uart.lsb_primero = 1;
        config_global.protocolo.ajustes.uart.invertido = 1; // lsb, inv
    } else if (config_global.protocolo.ajustes.uart.lsb_primero == 1 && config_global.protocolo.ajustes.uart.invertido == 1) {
        config_global.protocolo.ajustes.uart.lsb_primero = 0; // msb, inv
    } else {
        config_global.protocolo.ajustes.uart.lsb_primero = 1;
        config_global.protocolo.ajustes.uart.invertido = 0; // lsb, nrm
    }
    visor_necesita_redibujo_completo = 1;
    necesita_refresco = 1;
}
