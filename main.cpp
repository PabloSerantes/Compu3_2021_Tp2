#include<mbed.h>
#include<cstdlib>

/**
 * @brief Defino la cantidad de botones disponibles
 * 
 */

#define NROBOTONES    4

/**
 * @brief Defino la cantidad de leds disponibles
 * 
 */
#define NROLEDS       4

/**
 * @brief Tiempo para loop de leds random en el estado de idle
 * 
 */
#define MS1000    1000

/**
 * @brief Tiempo para loop de leds random en el estado de gameon
 * 
 */
#define MS300    300

#define TRUE    1
#define FALSE   0

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 * 
 */
#define INTERVAL    40

DigitalOut LED(PC_13);
BusOut leds(PB_12, PB_13, PB_14, PB_15);
BusIn boton(PB_9, PB_8, PB_7, PB_6);

/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,
    BUTTON_UP,
    BUTTON_FALLING,
    BUTTON_RISING
}_eButtonState;

/**
 * @brief Enumeración que indica el estado general del juego, idle es el juego detenido, gameon es
 * el ciclo inicio, give es el ciclo de muestra, recieve es el ciclo de entrada de tecla, lost rompe el ciclo y devuelve a 
 * idle una vez que se pierda.
 * 
 */
typedef enum{
    IDLE,
    GAMEON,
    GIVE,
    RECIEVE,
    LOST
}_eGameState;

/**
 * @brief Estructura que sirve para detectar el estado de los botones
 * 
 */
typedef struct{
    uint8_t state;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

/**
 * @brief Inicializa la MEF de botones
 * Le dá un estado inicial a los botones dependiendo del parametro que se le pase
 * 
 * @param indice define qué boton es
 */
void startMef(uint8_t indice);

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param indice Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef(uint8_t indice);

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 * @param indice indica de que led se trata
 */
void togleLed(uint8_t indice);

void heartbeater();
Ticker heartbeat;

_sTeclas ourButton[NROBOTONES];

uint16_t mask[] = {0x0001, 0x0002, 0x0004, 0x0008}; //!< Mascara que se aplica para cambiar el estado de los leds
uint16_t maskallornothing[] = {0x0000, 0x000F}; //!< Mascara para trabajar con todos los botones/leds al mismo tiempo
uint8_t estadoDelJuego = IDLE; //!< Selector de estado de la maquina de estado general
uint8_t loopCounter = 0; //!< cuentavueltas para determinar cantidad de ciclos necesarios
uint8_t given = FALSE; //!< indicador para saber si el topo salió
Timer miTimer; //!< Timer general
uint8_t molePosition; //!< Presenta la posicion matricial del topo
uint8_t hit = FALSE; //!< Bandera que indica si el topo fue golpeado correctamente

int tiempoMs = 0; //!< variable donde voy a almacenar el tiempo del timer una vez cumplido (para comprobar botones)
int tiempoMsLoop = 0; //!< variable donde voy a almacentar el tiempo del timer una vez cumplido un determinado ciclo

int main()
{
    miTimer.start();
    for(uint8_t indice = 0; indice < NROBOTONES; indice++){  
        startMef(indice);
    }
    leds = 0;
    LED = 0;
    heartbeat.attach(heartbeater, 0.25);
    while(TRUE)
    {
        srand(miTimer.read_us());

        switch(estadoDelJuego){
            case IDLE:   
                if(miTimer.read_ms() - tiempoMsLoop >= MS1000){
                    togleLed((rand()%NROLEDS));
                    tiempoMsLoop = miTimer.read_ms();
                }
                if ((miTimer.read_ms() - tiempoMs) >= MS1000){
                    tiempoMs = miTimer.read_ms();
                    for(uint8_t indice = 0; indice < NROBOTONES; indice++){                            
                        actuallizaMef(indice);
                    }
                }                             
            break;

            case GAMEON:
                if(loopCounter < 6){
                    if(miTimer.read_ms() - tiempoMsLoop >= MS300){
                        leds = leds ^ maskallornothing[1];
                        tiempoMsLoop = miTimer.read_ms();
                        loopCounter++;
                    }
                }else{
                    estadoDelJuego = GIVE;
                    loopCounter = 0;
                    tiempoMsLoop = miTimer.read_ms(); 
                    molePosition = (rand()%NROLEDS);
                    leds = maskallornothing[0];
                }
            break;

            case GIVE:                
                if((miTimer.read_ms() - tiempoMsLoop) >= MS1000*3){                                        
                    togleLed(molePosition);
                    given = TRUE;
                    estadoDelJuego = RECIEVE;
                    tiempoMsLoop = miTimer.read_ms(); 
                    hit = FALSE;
                }
                
                if ((miTimer.read_ms() - tiempoMs) > INTERVAL){
                    tiempoMs = miTimer.read_ms();
                    for(uint8_t indice = 0; indice < NROBOTONES; indice++){                            
                        actuallizaMef(indice);
                    }
                } 
                
                
            break;

            case RECIEVE:
                if(!hit){
                    if((miTimer.read_ms() - tiempoMsLoop) >= MS1000*3){                    
                        tiempoMsLoop = miTimer.read_ms(); 
                        leds = maskallornothing[0];
                        given = FALSE;
                        estadoDelJuego = LOST;
                    }

                    if ((miTimer.read_ms() - tiempoMs) > INTERVAL){
                        tiempoMs = miTimer.read_ms();
                        for(uint8_t indice = 0; indice < NROBOTONES; indice++){                            
                            actuallizaMef(indice);
                        }
                    }
                }else if (hit){
                    leds = maskallornothing[0];
                    estadoDelJuego = GAMEON;
                }
            break;

            case LOST:
                if(loopCounter < 7){
                    if((miTimer.read_ms() - tiempoMsLoop >= MS300) && (leds == maskallornothing[0])){
                        togleLed(molePosition);
                        tiempoMsLoop = miTimer.read_ms();
                        loopCounter++;
                    }else if((miTimer.read_ms() - tiempoMsLoop >= MS300) && (leds != maskallornothing[0])){
                        leds = maskallornothing[0];
                        tiempoMsLoop = miTimer.read_ms();
                        loopCounter++;
                    }
                }else{
                    loopCounter = 0;
                    tiempoMsLoop = miTimer.read_ms();
                    tiempoMs = miTimer.read_ms();
                    estadoDelJuego = IDLE;
                    leds = maskallornothing[0];
                }                
            break;
        }
    }
    return 0;
}



void startMef(uint8_t indice){
    ourButton[indice].state = BUTTON_UP;
}


void actuallizaMef(uint8_t indice){
    switch (ourButton[indice].state)
    {
    case BUTTON_DOWN:
        if(boton.read() & mask[indice]){
            ourButton[indice].state = BUTTON_RISING;

            if(estadoDelJuego == GIVE){
                if(given == FALSE){
                    tiempoMs = miTimer.read_ms();
                    tiempoMsLoop = miTimer.read_ms(); 
                    leds = maskallornothing[0];
                    estadoDelJuego = LOST;
                    ourButton[indice].state = BUTTON_UP;
                }                
            }

            if(estadoDelJuego == RECIEVE && !hit){
                if((boton.read() & mask[indice]) == leds){
                    hit = TRUE;
                    leds = maskallornothing[0];
                    ourButton[indice].state = BUTTON_UP;
                }else{
                    hit = FALSE;
                    leds = maskallornothing[0];
                    ourButton[indice].state = BUTTON_UP;
                    tiempoMs = miTimer.read_ms();
                    tiempoMsLoop = miTimer.read_ms(); 
                    estadoDelJuego = LOST;
                }
            }

        }
    break;
    case BUTTON_UP:
        if(!(boton.read() & mask[indice])){
            ourButton[indice].state = BUTTON_FALLING;
        }
    break;
    case BUTTON_FALLING:
        if(!(boton.read() & mask[indice]))
        {
            ourButton[indice].state = BUTTON_DOWN;
            //Flanco de bajada
            if(estadoDelJuego == IDLE){
                estadoDelJuego = GAMEON;
                tiempoMs = miTimer.read_ms();
                tiempoMsLoop = miTimer.read_ms(); 
                leds = maskallornothing[0];
                ourButton[indice].state = BUTTON_UP;
            }
        }else{
            ourButton[indice].state = BUTTON_UP;    
        }
    break;
    case BUTTON_RISING:
        if(boton.read() & mask[indice]){
            ourButton[indice].state = BUTTON_UP;

            //Flanco de Subida    
        }else{
            ourButton[indice].state = BUTTON_DOWN;
        }
    break;
    
    default:
        startMef(indice);
        break;
    }
}

void togleLed(uint8_t indice){
    leds = mask[indice];
}

void heartbeater(){
    LED = !LED;
}