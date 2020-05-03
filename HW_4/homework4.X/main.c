#include <xc.h>
#include <stdio.h>
#include <sys/attribs.h>
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

int main() {
    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;       // set pin A4 as output (LED)
    LATAbits.LATA4 = 1;         // start with pin A4 low (LED off)
    
    // initialize I2C
    i2c_master_setup();
    
    // initialize communication with SSD1306 and clear screen
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    
    __builtin_enable_interrupts();  // re-enable interrupts
    
    int t_LED = 0;                  // "heartbeat" counter; rolls over every 1/10 second to toggle LED
    int a = 0;                      // counter variable
    char count_message[50];         // array holding string to print
    char fps_message[50];
    
    while (1) {
        _CP0_SET_COUNT(0);                              // reset core timer

        sprintf(count_message, "Count = %d", a);        // create count string
        
        drawString(0, 0, count_message);                // print string to screen

        sprintf(fps_message, "FPS rate = %5.2f", (double)24000000/(double)_CP0_GET_COUNT());    // create FPS string
        
        drawString(0, 2, fps_message);                  // print string to screen
        
        a++;                                            // iterate counter
        
        
        if (t_LED > 24000000/10) {                      // if elapsed time exceeds 1/10 second
            LATAbits.LATA4 = !LATAbits.LATA4;           // toggle LED
            t_LED = 0;                                  // reset heartbeat counter
        }
        
        t_LED += _CP0_GET_COUNT();                      // add iteration time to heartbeat counter
    }
}