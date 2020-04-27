#include<xc.h>                      // processor SFR definitions
#include<sys/attribs.h>             // __ISR macro
#include"i2c_master_noint.h"        // I2C functions

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

unsigned char w_add = 0b01000000;       // write address
unsigned char r_add = 0b01000001;       // read address

void init_MCP23017() {
    // initialize I2C communication with MCP23017 chip

    i2c_master_setup();         // initialize I2C
    
    // set pin bus A to digital output
    i2c_master_start();         // start bit
    i2c_master_send(w_add);     // write address
    i2c_master_send(0x00);      // register (IODIRA)
    i2c_master_send(0x00);      // make all A pins output
    i2c_master_stop();          // stop bit
    
    // set pin bus B to input
    i2c_master_start();         // start bit
    i2c_master_send(w_add);     // write address
    i2c_master_send(0x01);      // register (IODIRB)
    i2c_master_send(0xFF);      // make all B pins input
    i2c_master_stop();          // stop bit
}

void setPin(unsigned char reg, int val, int pin){
    // uses I2C communication to set value of output pin on bus A
    
    i2c_master_start();             // start bit
    i2c_master_send(w_add);         // send write address
    i2c_master_send(reg);           // register (0x14 = OLATA, 0x15 = OLATB)
    i2c_master_send(val << pin);    // set value of pin
    i2c_master_stop();              // stop bit
}

unsigned char getPin(unsigned char reg) {
    // uses I2C communication to read input pins on bus B

    i2c_master_start();                             // start bit
    i2c_master_send(w_add);                         // send write address
    i2c_master_send(reg);                           // register (0x13=GPIOB)
    i2c_master_restart();                           // restart bit
    i2c_master_send(r_add);                         // send read address
    unsigned char read_B = i2c_master_recv();       // read byte
    i2c_master_ack(1);                              // acknowledge
    i2c_master_stop();                              // stop bit
    
    return read_B;
}

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
    LATAbits.LATA4 = 0;         // start with pin A4 low (LED off)
    
    // initialize I2C with MCP23017
    init_MCP23017();
    
    // start with LED off
    setPin(0x14, 0, 7);
    
    __builtin_enable_interrupts();

    int t_elapsed = 0;          // "heartbeat" counter; rolls over every 1/10 second to toggle LED
    
    while (1) {
        _CP0_SET_COUNT(0);                      // reset core timer
        
        unsigned char read_B = getPin(0x13);    // read bus B pins
               
        if (!(read_B & 0b00000001)) {
            setPin(0x14, 1, 7);                 // turn on LED if button pressed
        }
        else {
            setPin(0x14, 0, 7);                 // LED off if button not pressed
        }
        
        t_elapsed += _CP0_GET_COUNT();          // add iteration time to heartbeat counter
        
        if (t_elapsed > 24000000/10) {          // if elapsed time exceeds 1/10 second
            LATAbits.LATA4 = !LATAbits.LATA4;   // toggle LED
            t_elapsed = 0;                      // reset heartbeat counter
        }
    }
}