#ifndef INTERFACE_H
#define INTERFACE_H

#include "main.h"
#include "disp_ui.h"
#include "touch_driver.h"
#include "interface_conf.h"
// colores rgb565 estandar
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF

// paleta de tema de señales
#define COLOR_BG_DARK     0x10A2  // fondo carbon muy oscuro
#define COLOR_BG_DARKER   0x0842  // fondo de panel mas oscuro
#define COLOR_SIG_HIGH    0x07E0  // verde brillante - logica alta
#define COLOR_SIG_LOW     0x07E0  // verde brillante - nivel bajo
#define COLOR_SIG_LABEL   0xFFFF  // blanco - etiquetas canal
#define COLOR_SIG_SEP     0x2104  // separador gris oscuro sutil
#define COLOR_UI_BTN      0x2945  // boton azul-gris oscuro
#define COLOR_UI_BTN_ACT  0x07FF  // boton cian activo/presionado
#define COLOR_UI_ACCENT   0x07FF  // acento cian
#define COLOR_UI_STATUS   0x0124  // barra de estado muy oscura
#define COLOR_UI_STATUS_T 0xFFFF  // texto barra de estado
#define COLOR_UI_REC      0xF800  // rojo para boton rec




// estado interfaz
typedef enum {
	STATE_MAIN_MENU,
	STATE_VIEWER,
	STATE_SETTINGS,
	STATE_PROTOCOL_CONFIG,
	STATE_TABLE_VIEW,
} AppState_t;

// vistas
typedef struct {
    Button_t* botones;     // puntero a arreglo de botones
    uint8_t conteoBotones;   // cantidad botones
    void (*al_dibujar)(void);  // retrollamada de dibujo en pantalla
} View_t;

typedef struct {
    uint8_t muestras[LOGIC_NUM_CHANNELS][MAX_NUM_MUESTRAS];
    uint16_t muestras_totales;
    uint8_t canales_activos;
    const char *etiquetas_canal[LOGIC_NUM_CHANNELS];
} CaptureBuffer_t;

extern CaptureBuffer_t captura;
extern uint32_t desplazamiento_t_visor;
extern uint8_t zoom_visor;
extern int16_t desplazamiento_tabla;
extern uint8_t visor_necesita_redibujo_completo;
extern uint8_t modo_zoom_encoder;

extern AppState_t estado_actual;
extern uint8_t necesita_refresco;

extern Button_t botones_menu_principal[];
extern Button_t botones_ajustes[];
extern Button_t botones_visor[];
extern Button_t botones_config_protocolo[];
extern Button_t botones_vista_tabla[];
extern View_t vistas[];

// retrollamadas de botones

void AlClicCaptura(void);
void AlClicAtras(void);
void AlClicConfig(void);
void AlClicTabla(void);
void AlClicAlternarModo(void);
void AlClicAtrasConfig(void);
void AlClicDecodificar(void);
void AlClicAjuste(void);
void AlClicParamsUART(void);

void AlClicDesplazarIzq(void);
void AlClicDesplazarDer(void);
void AlDesplazarEncoderIzq(void);
void AlDesplazarEncoderDer(void);

void AlClicProtocolo(void);
void AlClicFrec(void);
void AlClicDisparo(void);
void AlClicRadix(void);

// vistas callbacks
void Dibujar_MenuPrincipal();
void Dibujar_Visor();
void Dibujar_Ajustes();
void Dibujar_ConfigProtocolo();
void Dibujar_VistaTabla();

#endif
