1. Cel projektu:
Celem projektu było stworzenie kalkulatora wykorzystującego mikrokontroler MKL05Z4,
klawiaturę oraz wyświetlacz LCD1602. Z założenia, taki projekt miał umożliwiać
wykonywanie podstawowych operacji matematycznych, czyli dodawanie, odejmowanie,
mnożenie, dzielenie. Taki program powinien operować na liczbach typu
zmiennoprzecinkowego, a do kasowania zawartości wyświetlacza miał się przysłużyć panel
dotykowy zawarty w płytce mikrokontrolera.

2. Funkcjonalność projektu:
Główną funkcjonalnością projektu jest możliwość wykonywania operacji dodawania,
odejmowanie, mnożenie I dzielenia. Mamy również obsługę wyświetlacza LCD1602
poprzez prezentowanie wprowadzonych danych oraz wyników działań. To wszystko nie
mogło by się obejść bez obsługi przerwań od klawiszy na klawiaturze matrycowej
podłączonej na odpowiednie piny mikrokontrolera. Dla usprawnienia pracy z takim
kalkulatorem została stworzona funkcjonalność dla panelu dotykowego(TSI), która
odpowiada za czyszczenie kalkulatora poprzez dotknięcie palcem dotykowej powierzchni.
Dla pozbycia się niechcianego efektu BOUNCE(drgania zestyków), czyli zjawiska
występującego podczas fizycznego naciskania przycisku, zrobiono specjalny mechanizm
do eliminacji zakłóceń. Gdyby nie to, mikrokontroler mógłby zinterpretować jedno
naciśnięcie klawisza jako wiele. Stąd też zawarte w kodzie jest opóźnienie
(DEBOUNCE_DELAY), aby przez czas jego trwania mikrokontroler nie reagował na
ewentualne sygnały od klawisza. Dla bezpieczeństwa zrobione zostało wykrywanie I
sygnalizacja dzielenia przez zero poprzez wypisanie na ekranie komunikatu “err”.

3. Opis działania:
Na początku działania programu wyświetla się ekran powitalny z komunikatem: „Witaj w
kalkulatorze”. Po upływie czasu opóźnienia zdanie to znika i przechodzimy do postawowej
obsługi działań. Obsługa klawiatury została zrealizowana przy użyciu przerwań PORTA oraz
mechanizmu cyklicznego przełączania aktywnych rzędów klawiatury, sterowanego również
przez licznik SysTick. Każde wciśnięcie klawisza powoduje wygenerowanie odpowiedniego
sygnału przerwania oraz odczyt załączonego rzędu klawiatury. Tym samym program
najpierw pobiera od nas pierwszą liczbę(zawartą w tablicy4x4) i czeka na podanie
operatora. Aby był widoczny typ wykonywanej operacji przez pewien czas wyświetla się
znak działania, lecz gdy zgaśnie podajemy drugą liczbę i naciskamy klawisz zwracający
wynik działania( znak „=”, kryje się pod klawiszem S16 na klawiaturze matrycowej). Wynik
otrzymujemy z dokładnością do 5 miejsc po przecinku. W momencie, gdy chcemy przejść
do następnego działania, dotykamy pole read slidera. To działanie sprawia natychmiastowe
skasowanie zawartości wyświetlacza (w między czasie na krótko wypisuje również
komunikat „cleared”). Wprowadzone znaki wyświetlają się na ekranie LCD1602. W
przypadku błędu dzielenia przez zero, na ekranie pojawia się komunikat „err”.
