#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "tsi.h"


// PORTA - definicje pinów klawiatury
#define C_0 0
#define C_1 10
#define C_2 11
#define C_3 12

#define R_0 5
#define R_1 6
#define R_2 7
#define R_3 8

#define C_MASK ((1 << C_0) | (1 << C_1) | (1 << C_2) | (1 << C_3))			//maski okreslaja, jakie piny kolumny i wierszy sa aktywowane w czasie dzialania programu
#define R_MASK ((1 << R_0) | (1 << R_1) | (1 << R_2) | (1 << R_3))


//wartosci klawiatury
volatile static uint8_t activeRow = 0;
static const char KALKULATOR[4][4] = {
    {'+', '-', '/', '*'},
    {'0', '1', '2', '3'},
    {'4', '5', '6', '7'},
    {'8', '9', '.', '='}
};
volatile static int blad_dzielenia_przez_zero = 0;											//Flaga bledu dzielenia przez zero
volatile static char kalkulator_value = '0';														//przechowuje ostatnio nacisniety klawisz przez uzytkownika	
static char bufor_znakow[17] = "";  																		//Bufor do przechowywania wprowadzonych znaków(max 16 znaków + '\0')
static uint8_t nowe_znaki = 0;      																		//Liczba znakow w buforze
volatile static uint32_t currentTime = 0; 															//Aktualny czas (w ms)
volatile static uint32_t lastKeyPressTime = 0; 													//Czas ostatniego nacisniecia klawisza (uzywane do eliminacji dla debounce)

#define DEBOUNCE_DELAY 50 																							//Opóznienie w ms do eliminacji efektu bounce

static char operator = '\0';       																			//przechowuje wybrany operator(+,-,/,*), domyslnie jest pustym znakiem
static double pierwsza_liczba = 0;         															//Pierwszy operand
static double druga_liczba = 0;         																//Drugi operand
static int wybor_operatora = 0;        																	//Flaga, czy operator zostal wybrany(gdy flaga=0=>kalkulator zapisuje wartosc jako pierwsza liczba)
static int wprowadzanie_ujemnej_liczby = 0;															//flaga wskazujaca, czy wpisywany jest ujemny znak liczby

// Funkcja do wykonania operacji matematycznych
float wykonajOperacje(float a, float b, char num) {											//num=argument przekazujacy operator
    switch (num) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
				case '/': 
            if (b != 0) {
                return a / b;
            } else {
                blad_dzielenia_przez_zero=1;														//ustawienie flagi bledu
								LCD1602_SetCursor(0, 0); 																
                LCD1602_ClearAll();
                LCD1602_Print("Err");
								return 0;																								//zwracamy bezpieczna wartosc, ale komunikat sie pojawi
						}
		 default: 
			 return 0;
	}
}
						
// Funkcja do pobrania wartosci klawisza
void pobierz_klawisz(uint8_t col, uint8_t row);
void pobierz_klawisz(uint8_t col, uint8_t row) {
    if ((currentTime - lastKeyPressTime) < DEBOUNCE_DELAY) {
        return; 																												// Zignoruj sygnal klawisza, jesli debounce jeszcze trwa
    }

    lastKeyPressTime = currentTime; 																		// Aktualizacja czasu ostatniego nacisniecia

    char key = KALKULATOR[row][col];
    if (key == '=') {
			
        if (wybor_operatora && nowe_znaki > 0) {
            druga_liczba = atof(bufor_znakow); 												 	// Konwersja drugiego operandu
            double result = wykonajOperacje(pierwsza_liczba, druga_liczba, operator);
            snprintf(bufor_znakow, 16, "%.5f", result); 								// Wyswietlenie wyniku z dokladnoscia do 5-tego miejsca po przeecinku. Jesli potrzeba wiekszej dokladnosci mozna w tej lini zwiekszyc wartosc cyfr po przecinku
            nowe_znaki = strlen(bufor_znakow);
            wybor_operatora = 0; 																				// Reset operatora
        }
    } else if (key == '+' || key == '-' || key == '/' || key == '*') {
       
        if (!wybor_operatora && nowe_znaki > 0) {
            pierwsza_liczba = atof(bufor_znakow); 											// Konwersja pierwszego operandu
            operator = key;              																// Ustawienie operatora
            wybor_operatora = 1;             														// Flaga operatora
            nowe_znaki = 0;              																// Reset bufora
            bufor_znakow[0] = '\0';	
					
						LCD1602_ClearAll();
						char pokaz_operatora[17];
						snprintf(pokaz_operatora, sizeof(pokaz_operatora), "%.2f %c", pierwsza_liczba, operator);
						LCD1602_Print(pokaz_operatora);
						DELAY(10000);
        }
				else if (!wybor_operatora && nowe_znaki == 0 && key == '-') {
            // Interpretacja znaku minus dla pierwszej liczby
            wprowadzanie_ujemnej_liczby = 1;
            bufor_znakow[nowe_znaki++] = key;
            bufor_znakow[nowe_znaki] = '\0';
        }
				else if (wybor_operatora && nowe_znaki == 0 && key == '-') {
            // Interpretacja minusa dla drugiej liczby
            wprowadzanie_ujemnej_liczby = 1;
            bufor_znakow[nowe_znaki++] = key;
            bufor_znakow[nowe_znaki] = '\0';
        }
    } else {
        // Dodanie klawisza do bufora (jesli jest miejsce)
        if (nowe_znaki < 16) {
            bufor_znakow[nowe_znaki++] = key;
            bufor_znakow[nowe_znaki] = '\0'; 														// Utrzymanie poprawnego zakonczenia stringa
        }
    }
    kalkulator_value = key;
}

// Obsluga przerwania PORTA (dla kolumn klawiatury)
void PORTA_IRQHandler(void) {
    uint32_t ISFRbuff = (PORTA->ISFR & C_MASK); 												// Sprawdzenie, która kolumna zostala aktywowana

    switch (ISFRbuff) {
        case (1 << C_0):
            pobierz_klawisz(0, activeRow);
            break;
        case (1 << C_1):
            pobierz_klawisz(1, activeRow);
            break;
        case (1 << C_2):
            pobierz_klawisz(2, activeRow);
            break;
        case (1 << C_3):
            pobierz_klawisz(3, activeRow);
            break;
        default:
            break;
    }

    PORTA->ISFR |= C_MASK; 																							// Czytanie flagi przerwan
    NVIC_ClearPendingIRQ(PORTA_IRQn);
}

void clearCalculator() {
    memset(bufor_znakow, 0, sizeof(bufor_znakow));  										// Wyczysc bufor
    nowe_znaki = 0;  																										// Reset indeksu bufora
    operator = '\0'; 																										// Reset operatora
    pierwsza_liczba = 0;
    druga_liczba = 0;
    wybor_operatora = 0;
		blad_dzielenia_przez_zero = 0;
		wprowadzanie_ujemnej_liczby = 0;
}

// Obsluga SysTick (przelaczanie aktywnego wiersza i inkrementacja czasu)
void SysTick_Handler(void) {
    activeRow = (activeRow + 1) % 4;  																	// Przelaczanie wiersza

    PTA->PDOR |= R_MASK;                															 // Wylaczanie wszystkich wierszy
    PTA->PDOR &= ~(1 << (R_0 + activeRow));  													// Aktywacja biezacego wiersza

    currentTime++; 																										// Zwiekszenie aktualnego czasu o 1 ms
}


int main(void) {
		
		SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;  															// Wlaczenie portu A
		SIM->SCGC5 |= SIM_SCGC5_TSI_MASK;
	
    // Konfiguracja pinów PORTA (MUX na GPIO)
    PORTA->PCR[C_0] |= PORT_PCR_MUX(1);
    PORTA->PCR[C_1] |= PORT_PCR_MUX(1);
    PORTA->PCR[C_2] |= PORT_PCR_MUX(1);
    PORTA->PCR[C_3] |= PORT_PCR_MUX(1);

    PORTA->PCR[R_0] |= PORT_PCR_MUX(1);
    PORTA->PCR[R_1] |= PORT_PCR_MUX(1);
    PORTA->PCR[R_2] |= PORT_PCR_MUX(1);
    PORTA->PCR[R_3] |= PORT_PCR_MUX(1);

    // Konfiguracja kolumn jako wejsc z rezystorami pull-up
    PORTA->PCR[C_0] |= (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);
    PORTA->PCR[C_1] |= (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);
    PORTA->PCR[C_2] |= (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);
    PORTA->PCR[C_3] |= (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);

    // Konfiguracja przerwan dla kolumn
    PORTA->PCR[C_0] |= PORT_PCR_IRQC(0xa);  												// Falling edge
    PORTA->PCR[C_1] |= PORT_PCR_IRQC(0xa);
    PORTA->PCR[C_2] |= PORT_PCR_IRQC(0xa);
    PORTA->PCR[C_3] |= PORT_PCR_IRQC(0xa);

    NVIC_SetPriority(PORTA_IRQn, 3);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);

    // Konfiguracja pinów wierszy jako wyjsc
    PTA->PDDR |= R_MASK;
    PTA->PDOR |= R_MASK;

    // Inicjalizacja wyswietlacza, panelu dotykowego
    
		LCD1602_Init();
    LCD1602_Backlight(TRUE);
		TSI_Init();
    
		LCD1602_ClearAll();
		LCD1602_Print("     Witaj w");
		LCD1602_SetCursor(3,1);
		LCD1602_Print(   "kalkulatorze");
		DELAY(10500);
		LCD1602_ClearAll();
		
		uint8_t w =0;
		
    // Konfiguracja SysTick
    SysTick_Config(SystemCoreClock / 100);  											// Przerwanie co 1 ms

    // Glówna petla
    while (1) 
		{
			w=TSI_ReadSlider();	
			if(w!=0)
			{
				clearCalculator();
				LCD1602_ClearAll();
				LCD1602_Print("cleared");
				DELAY(1000);
				LCD1602_ClearAll();
			}
			if(blad_dzielenia_przez_zero){
							continue;
						}
				LCD1602_ClearAll();
        LCD1602_Print(bufor_znakow);  														// Wyswietlanie zawartosci bufora
        DELAY(250);																								// 0.25 sekundy			
		}
}


