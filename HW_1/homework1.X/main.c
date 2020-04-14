#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

// DEVCFG0
#pragma config DEBUG = 0b11             // disable debugging
#pragma config JTAGEN = OFF             // disable jtag
#pragma config ICESEL = 0b11            // use PGED1 and PGEC1
#pragma config PWP = 0b111111111        // disable flash write protect
#pragma config BWP = 1                  // disable boot write protect
#pragma config CP = 1                   // disable code protect

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
#pragma config USERID = 0       // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = 0     // allow multiple reconfigurations
#pragma config IOL1WAY = 0      // allow multiple reconfigurations

int main() {
    int i;                      // on/off cycle counter

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
    LATAbits.LATA4 = 0;         // start with pin A4 low (LED off)
    TRISBbits.TRISB4 = 1;       // set pin B4 as input (USER button)

    __builtin_enable_interrupts();

    while (1) {
        // use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing
        // remember the core timer runs at half the sysclk
        if (!PORTBbits.RB4){                                    // if USER buttor is pushed
            for (i = 0; i < 2; i++) {                           // run loop twice
                _CP0_SET_COUNT(0);                              // clear core timer
                LATAbits.LATA4 = 1;                             // turn LED on
                while (_CP0_GET_COUNT() < 24000000/2) {;}       // wait 0.5 second
                _CP0_SET_COUNT(0);                              // clear core timer
                LATAbits.LATA4 = 0;                             // turn LED off
                while (_CP0_GET_COUNT() < 24000000/2) {;}       // wait 0.5 second
            }
        }
    }
}