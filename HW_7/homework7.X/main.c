#include <stdio.h>
#include "i2c_master_noint.h"
#include "font.h"
#include "adc.h"
#include "ws2812b.h"

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
#pragma config FPLLIDIV = DIV_2     // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24     // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2     // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0           // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = 0         // allow multiple reconfigurations
#pragma config IOL1WAY = 0          // allow multiple reconfigurations

int t_LED = 0;                      // heartbeat counter
int numLEDs = 3;                    // number of addressable LEDs

void heartbeat() {
    /*
        Function used to blink system LED at 5Hz
     */
    t_LED += _CP0_GET_COUNT();      // add iteration time to heartbeat counter
    
    if (t_LED > 24000000/10) {
        LATAbits.LATA4 = !LATAbits.LATA4;       // toggle LED after 1/10 second
        t_LED = 0;                              // reset counter
    }
    
    _CP0_SET_COUNT(0);
}

int ctmu_sum(int pin, int delay, int n) {
    /*
        Read a CTMU pin a specified number of times and sum all of the reads
     */
    int i = 0;                  // counter
    int sum = 0;                // sum of all CTMU reads
    
    // compute sum of n CTMU read values
    for (i = 0; i < n; i++) {
        sum += ctmu_read(pin, delay);
    }
    
    return sum;
}

void turn_on_LED(float * brightness, int LED, float level) {
    /*
        Function sets brightness level of each LED
     */
    int i;                              // counter
    
    for (i = 0; i < numLEDs; i++) {     // loop through all LEDs
        if (i == LED) {                 // set indicated LED brightness to appropriate level
            brightness[i] = level;
        } 
        else {                          // set all other LED brightnesses to 0
            brightness[i] = 0.0;
        }
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

    // set up system heartbeat LED
    TRISAbits.TRISA4 = 0;           // set pin A4 as output (LED)
    LATAbits.LATA4 = 0;             // start with pin A4 low (LED off)
    
    // initialize ADC and CTMU
    adc_setup();
    ctmu_setup();
    
    // initialize addressable LEDs
    ws2812b_setup();
    
    // initialize I2C
    i2c_master_setup();
    
    // initialize screen
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    
    // re-enable interrupts
    __builtin_enable_interrupts();
    
    // CTMU pins
    int ctmu_pin1 = 4;          // CTMU pin 1 (AN4)
    int ctmu_pin2 = 5;          // CTMU pin 2 (AN5)

    // CTMU read values
    int ctmu_1;                 // first CTMU pin value
    int ctmu_2;                 // second CTMU pin value
    
    // messages to print
    char ctmu_str1[12];         // first CTMU read message
    char ctmu_str2[12];         // second CTMU read message
    
    //addressable LED info
    wsColor colors[numLEDs];            // array of RGB color values
    int hues[] = {120, 0, 0};           // hues for each LED (green, red, white)
    int saturation[] = {1, 1, 0};       // color saturation for each LED
    float brightness[numLEDs];          // brightness values array
    int LED;                            // LED to illuminate
    float level;                        // brightness level
    
    char message[10];                   // message holding brightness level
    
    int i;                              // counter variable
    
    while (1) {
        heartbeat();
        
        // CTMU pin reads
        ctmu_1 = ctmu_sum(ctmu_pin1, 15, 10);
        ctmu_2 = ctmu_sum(ctmu_pin2, 15, 10);
        
        // print CTMU values to strings
        sprintf(ctmu_str1, "%d", ctmu_1);
        sprintf(ctmu_str2, "%d", ctmu_2);
        
        // display values on screen
        drawString(2, 1, ctmu_str1);
        drawString(2, 2, ctmu_str2);
        
        // illuminate LEDs based on which contacts are touched
        if (ctmu_1 < 6000 && ctmu_2 > 6000) {       // contact 1
            LED = 0;                                // first LED (green)
            level = 1.0;                            // full brightness
        }
        else if (ctmu_1 > 6000 && ctmu_2 < 6000) {  // contact 2
            LED = 1;                                // second LED (red)
            level = 1.0;                            // full brightness
        }
        else if (ctmu_1 < 6000 && ctmu_2 < 6000) {  // both contacts touched
            LED = 2;                                // third LED (white)
            level = (ctmu_1 + ctmu_2)/9000.0;
            
            sprintf(message, "%.2f", level);        // print brightness level to string
            drawString(10, 2, message);             // display brightness level on screen
        }
        else {                                      // no contacts touched
            LED = 4;                                // all LEDs off
            level = 0;
            
            ssd1306_clear();                        // clear screen
        }
        
        turn_on_LED(brightness, LED, level);        // set brightness of all LEDs
        
        // convert HSB values for each LED to RGB levels
        for (i = 0; i < numLEDs; i++) {
            colors[i] = HSBtoRGB(hues[i], saturation[i], brightness[i]);
        }
        
        ws2812b_setColor(colors, numLEDs);          // illuminate LED
    }
}