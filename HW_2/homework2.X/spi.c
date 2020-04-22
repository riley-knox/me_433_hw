#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <math.h>
#include "spi.h"

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

unsigned short create_spi_write(unsigned char channel, unsigned short voltage) {
    /*
     Assembles 16-bit SPI write number
     
     channel = 0/1 (0 = output A, 1 = output B) [bit 15]
      
     buffer control = 1 [bit 14]
     output gain = 1 [bit 13]
     output shutdown control = 1 [bit 12]
      
     voltage = 12-bit output voltage [bits 0-11]
     */
    unsigned short write = (channel << 15);             // bit 15
    
    write = write | (0b111 << 12);                      // bits 14-12
    
    write = write | voltage;                            // bits 11-0
    
    return write;
}

void triangle_wave(unsigned short * wave) {
    int i;
    
    for (i = 0; i < 4096; i++) {
        wave[i] = i;
        wave[8192-(i+1)] = i;
    }
}

void sine_wave(unsigned short * wave) {
    int i;
    
    for (i = 0; i < 4096; i++) {
        wave[i] = wave[i + 4096] = 2047*sin(((2*M_PI)/4096)*i) + 2047;
    }
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
    TRISBbits.TRISB4 = 1;       // set pin B4 as input (USER button)
    
    initSPI();                  // initialize SPI1 communication

    __builtin_enable_interrupts();
    
    unsigned short t_wave[8192];
    unsigned short s_wave[8192];

    triangle_wave(t_wave);      // calculate triangle wave voltages
    sine_wave(s_wave);          // calculate sine wave voltages
    
    while (1) {
        int a;              // counter
        
        for (a = 0; a < 8192; a++) {
            _CP0_SET_COUNT(0);          // reset core timer
            
            unsigned short v_t = create_spi_write(0, t_wave[a]);        // triangle wave SPI write
            unsigned short v_s = create_spi_write(1, s_wave[a]);        // sine wave SPI write
            
            // write triangle wave voltage
            LATAbits.LATA1 = 0;         // CS low
            spi_io(v_t >> 8);           // MSB
            spi_io(v_t);                // LSB
            LATAbits.LATA1 = 1;         // CS high
            
            // write sine wave voltage
            LATAbits.LATA1 = 0;         // CS low
            spi_io(v_s >> 8);           // MSB
            spi_io(v_s);                // LSB
            LATAbits.LATA1 = 1;         // CS high
        
            while (_CP0_GET_COUNT() < 24000000/8192) {;}                // delay 1/8192 second
        }
    }
}

// initialize SPI1
void initSPI() {
    // Pin B14 has to be SCK1
    // Turn off analog pins
    ANSELA = 0;
    ANSELB = 0;
    
    // Make an output pin for CS
    TRISAbits.TRISA1 = 0;           // set pin 3 as output
    LATAbits.LATA1 = 1;             // set output high
    
    // Set SDO1
    RPB5Rbits.RPB5R = 0b0011;       // RPB5
    
    // Set SDI1

    // setup SPI1
    SPI1CON = 0;                // turn off the SPI module and reset it
    SPI1BUF;                    // clear the RX buffer by reading from it
    SPI1BRG = 1;             // 1000 for 24kHz, 1 for 12MHz; // baud rate to 10 MHz [SPI1BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0;    // clear the overflow bit
    SPI1CONbits.CKE = 1;        // data changes when clock goes from high to low (since CKP is 0)
    SPI1CONbits.MSTEN = 1;      // master operation
    SPI1CONbits.ON = 1;         // turn on SPI 
}

unsigned char spi_io(unsigned char o) {
    SPI1BUF = o;                        // load data into buffer
    
    while (!SPI1STATbits.SPIRBF) {      // send buffer out/read new buffer in
        ;
    }
    
    return SPI1BUF;                     // return whatever was read into the buffer
}