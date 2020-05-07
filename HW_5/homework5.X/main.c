#include <stdio.h>
#include <sys/attribs.h>
#include "ws2812b.h"
#include "font.h"

// DEVCFG0
#pragma config DEBUG = 0b11         // disable debugging
#pragma config JTAGEN = OFF         // disable jtag
#pragma config ICESEL = 0b11        // use PGED1 and PGEC1
#pragma config PWP = 0b111111111    // disable flash write protect
#pragma config BWP = 1              // disable boot write protect
#pragma config CP = 1               // disable code protect

// DEVCFG1
#pragma config FNOSC = 0b011        // use primary oscillator with pll
#pragma config FSOSCEN = 0          // disable secondary oscillator
#pragma config IESO = 0             // disable switching clocks
#pragma config POSCMOD = 0b10       // high speed crystal mode
#pragma config OSCIOFNC = 1         // disable clock output
#pragma config FPBDIV = 0b00        // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = 0b11         // disable clock switch and FSCM
#pragma config WDTPS = 0b10100      // use largest wdt
#pragma config WINDIS = 1           // use non-window mode wdt
#pragma config FWDTEN = 0           // wdt disabled
#pragma config FWDTWINSZ = 0b11     // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = 0b001     // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = 0b111      // multiply clock after FPLLIDIV
#pragma config FPLLODIV = 0b001     // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0           // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = 0         // allow multiple reconfigurations
#pragma config IOL1WAY = 0          // allow multiple reconfigurations

void setLEDs(float * hues, int numLEDs) {
    int a;                              // LED counter
    
    for (a = 0; a < numLEDs; a++) {     // loop through all LEDs
        hues[a]++;                      // increase hue value by 1
        if (hues[a] == 360) {
            hues[a] = 0;                // reset hue to 0 if it hits end
        }
    }
}

void allColors(wsColor * colors, float * hues, int numLEDs) {
    float saturation = 1.0, brightness = 0.5;        // full saturation/brightness
    int i;                                           // LED counter
    
    setLEDs(hues, numLEDs);         // set hue values for each LED
    
    for (i = 0; i < numLEDs; i++) {                                 // loop through all LEDs
        colors[i] = HSBtoRGB(hues[i], saturation, brightness);      // set RGB value for each LED
    }
}

int main() {
    // disable interrupts while initializing things
    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // set up system LED
    TRISAbits.TRISA4 = 0;           // set pin A4 as output (LED)
    LATAbits.LATA4 = 0;             // start with pin A4 low (LED off)
    
    // initialize addressable LEDs
    ws2812b_setup();
    
    // initialize I2C and screen
    i2c_master_setup();
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    
    // re-enable interrupts
    __builtin_enable_interrupts();
    
    int numLEDs = 4;                // number of addressable LEDs

    wsColor colors[numLEDs];        // array of RGB color values
    float hues[numLEDs];            // array to hold hue values for HSBtoRGB function

    // start LEDs at multiples of 90
    int b;                              // counter
    for (b = 0; b < numLEDs; b++) {
        hues[b] = 90.0*b;               // fill hues array with multiples of 90
    }
    
    // message to print
    char message[50];
    sprintf(message, "Flashing Lights - Kanye West ft. Dwele");
    
    int t_LED = 0;                              // heartbeat counter

    while (1) {
        _CP0_SET_COUNT(0);                      // reset core timer
        
        drawString(1, 1, message);              // write message on screen
    
        allColors(colors, hues, numLEDs);       // get RGB values for each color to 
        
        ws2812b_setColor(colors, numLEDs);      // illuminate LEDs
        
        t_LED += _CP0_GET_COUNT();              // add iteration time to heartbeat counter
        
        if (t_LED > 24000000/10) {
            LATAbits.LATA4 = !LATAbits.LATA4;   // toggle LED if timer exceeds 1/10 second
            t_LED = 0;                          // reset counter
        }
    }
}