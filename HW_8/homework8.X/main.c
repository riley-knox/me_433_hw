#include <stdio.h>
#include "i2c_master_noint.h"
#include "font.h"
#include "rtcc.h"

// DEVCFG0
#pragma config DEBUG = 0b11         // disable debugging
#pragma config JTAGEN = OFF         // disable jtag
#pragma config ICESEL = 0b11        // use PGED1 and PGEC1
#pragma config PWP = 0b111111111    // disable flash write protect
#pragma config BWP = 1              // disable boot write protect
#pragma config CP = 1               // disable code protect

// DEVCFG1
#pragma config FNOSC = 0b011        // use primary oscillator with pll
#pragma config FSOSCEN = 1          // enable secondary oscillator
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
#pragma config FPLLIDIV = DIV_2     // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24     // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2     // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0           // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = 0         // allow multiple reconfigurations
#pragma config IOL1WAY = 0          // allow multiple reconfigurations

int t_LED = 0;                      // heartbeat counter

void heartbeat() {
    /*
        Function used to blink system LED at 5Hz
     */
    t_LED += _CP0_GET_COUNT();      // add iteration time to heartbeat counter
    
    if (t_LED > 24000000/10) {
        LATBbits.LATB5 = !LATBbits.LATB5;       // toggle LED after 1/10 second
        t_LED = 0;                              // reset counter
    }
    
    _CP0_SET_COUNT(0);              // reset core timer
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

    // set up system heartbeat LED
    TRISBbits.TRISB5 = 0;           // set pin B5 as output
    LATBbits.LATB5 = 0;             // start with pin B5 low (LED off)
    
    // set up system USER button
    TRISBbits.TRISB6 = 1;           // set pin B6 as input
    
    // initialize I2C
    i2c_master_setup();
    
    // initialize screen
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    
    // re-enable interrupts
    __builtin_enable_interrupts();
    
    // initialize RTCC peripheral - CHANGE THESE BASED ON DATE/TIME
    unsigned long time = 0x20270000;        // current time
    unsigned long date = 0x20060202;        // current date
    rtcc_setup(time, date);                 // "seed" RTCC with current time & date
    
    // initialize Timer2 to update screen at 2Hz
    T2CONbits.TCKPS = 0b111;    // 256:1 prescale
    T2CONbits.TCS = 0;          // use internal peripheral clock
    T2CONbits.T32 = 1;          // Timer2 as 32-bit timer
    T2CONbits.ON = 1;           // turn on Timer2
    
    int a = 0;              // counter variable
    unsigned char pos;      // numeric day of the week
    char day[12];           // name of day of the week
    char count[12];         // count message to display
    char hr_min_sec[10];    // time message to display
    char day_date[20];      // date message to display
    
    TMR2 = 0;               // set Timer2 to 0
    while (1) {
        heartbeat();
        
        rtccTime time = readRTCC();     // get time/date information
        
        pos = time.wk;                  // get numeric day of the week
        dayOfTheWeek(pos, day);         // get name of day of the week from number
        
        if (TMR2 > 93750) {             // update screen every 0.5 seconds
            TMR2 = 0;                   // reset Timer2
            
            sprintf(count, "%d", a);    // write count to string
            sprintf(hr_min_sec, "%d%d:%d%d:%d%d", time.hr10, time.hr01, time.min10, time.min01, time.sec10, time.sec01);        // write time to string
            sprintf(day_date, "%s, %d%d/%d%d/20%d%d", day, time.mn10, time.mn01, time.dy10, time.dy01, time.yr10, time.yr01);   // write day & date to string
            
            // screen displays
            drawString(5, 0, count);            // count
            drawString(3, 2, hr_min_sec);       // time
            drawString(3, 3, day_date);         // weekday
            
            a++;                                // iterate count
        }
    }
}