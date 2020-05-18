#include <stdio.h>
#include "imu.h"
//#include "ssd1306.h"
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
#pragma config FPLLIDIV = DIV_2     // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24     // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2     // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0           // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = 0         // allow multiple reconfigurations
#pragma config IOL1WAY = 0          // allow multiple reconfigurations

int t_LED = 0;                      // heartbeat counter

unsigned char w_add = 0b11010110;   // IMU write address
unsigned char r_add = 0b11010111;   // IMU read address

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

void incline(int x, int y){
    /* 
        Draw inclinometer based on x- and y-axis accelerations returned from IMU
     */
    ssd1306_clear();            // clear screen
    
    int a = 0, b = 0;           // counter variables
    
    // size of bars to draw, in number of pixels
    double x_bar = 16.0*((double)x/(double)16384);
    double y_bar = 64.0*((double)y/(double)16384);
    
    // vertical bar for x-axis acceleration
    if (x_bar < 0) {                    // upwards from center if acceleration negative
        for (a = 0; a < 16; a++) {
            if (a > 16 + (int)x_bar) {
                ssd1306_drawPixel(64, a, 1);
            }
        }
    } else {                            // downwards if acceleration positive
        for (a = 16; a < 32; a++) {
            if (a < 16 + (int)x_bar) {
                ssd1306_drawPixel(64, a, 1);
            }
        }
    }
    
    // horizontal bar for y-axis
    if (y_bar > 0) {                    // left from center if acceleration positive
        for (b = 0; b < 64; b++) {
            if (b > 64 - (int)y_bar) {
                ssd1306_drawPixel(b, 16, 1);
            }
        }
    } else {                            // right if acceleration negative
        for (b = 64; b < 128; b++) {
            if (b < 64 - (int)y_bar) {
                ssd1306_drawPixel(b, 16, 1);
            }
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

    // set up system LED
    TRISAbits.TRISA4 = 0;           // set pin A4 as output (LED)
    LATAbits.LATA4 = 0;             // start with pin A4 low (LED off)
    
    // initialize I2C
    i2c_master_setup();
    
    // initialize IMU
    init_LSM6DS33();
    
    // initialize screen
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    
    // re-enable interrupts
    __builtin_enable_interrupts();
    
    signed short imu_data[7];               // array to hold IMU data
    
    char gyro[50];              // gyroscope data string
    char acc[50];               // accelerometer data string
    char temp[50];              // temperature data string
    
    while (1) {
        heartbeat();
        
        imu_read(IMU_OUT_TEMP_L, imu_data, 7);
        
        /*
        // use this block to print out gyroscope/accelerometer/temperature data
        
        sprintf(gyro, "g: %d %d %d  ", imu_data[1], imu_data[2], imu_data[3]);
        sprintf(acc, "a: %d %d %d  ", imu_data[4], imu_data[5], imu_data[6]);
        sprintf(temp, "t: %d  ", imu_data[0]);
        
        drawString(2, 0, gyro);
        drawString(2, 1, acc);
        drawString(2, 2, temp);
        */
        
        incline(imu_data[4], imu_data[5]);
        
        ssd1306_update();
    }
}