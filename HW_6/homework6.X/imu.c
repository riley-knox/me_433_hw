#include "imu.h"
#include "i2c_master_noint.h"

void init_LSM6DS33() {
    // initialize the IMU chip
    // IMPORTANT: must initialize I2C communication elsewhere, this function is 
    // only for the IMU
    
    unsigned char who = 0;
    
    // read from WHO_AM_I register to confirm I2C working
    i2c_master_start();                         // start bit
    i2c_master_send(IMU_W_ADDR);                // send write address
    i2c_master_send(IMU_WHOAMI);                // address of WHO_AM_I register                  
    i2c_master_restart();                       // restart bit
    i2c_master_send(IMU_R_ADDR);                // read address
    who = i2c_master_recv();                    // read byte
    i2c_master_ack(1);                          // acknowledge
    i2c_master_stop();                          // stop bit
    
    if (who != 0b01101001) {                    // if returned address is wrong
        LATAbits.LATA4 = 1;                     // turn on LED
        while (1) {;}                           // enter infinite loop
    }
    
    // turn on accelerometer
    // initialize with 1.66kHz sample rate, 2g sensitivity, 100Hz filter
    // see datasheet for values to write
    i2c_master_start();                 // start bit
    i2c_master_send(IMU_W_ADDR);        // send write address
    i2c_master_send(IMU_CTRL1_XL);      // CTRL1_XL register
    i2c_master_send(0b10000010);        // send parameters
    i2c_master_stop();                  // stop bit
    
    // turn on gyroscope
    // initialize with 1.66kHz sample rate, 1000 dps sensitivity
    // see datasheet for values to write
    i2c_master_start();                 // start bit
    i2c_master_send(IMU_W_ADDR);        // send write address
    i2c_master_send(IMU_CTRL2_G);       // CTRL2_G register
    i2c_master_send(0b10001000);        // send parameters
    i2c_master_stop();                  // stop bit
    
    // enable multiple register read
    // see datasheet
    i2c_master_start();                 // start bit
    i2c_master_send(IMU_W_ADDR);        // send write address
    i2c_master_send(IMU_CTRL3_C);       // CTRL3_C register
    i2c_master_send(0b00000100);        // send parameters
    i2c_master_stop();                  // stop bit
}

void imu_read(unsigned char reg, signed short * data, int len) {
    int i = 0;                              // counter variable
    
    unsigned char vals[len*2];              // array to hold bits returned from I2C
    
    // read multiple from IMU
    i2c_master_read_multiple(reg, IMU_W_ADDR, IMU_R_ADDR, vals, len*2);
    
    // turn chars into shorts via bitwise operations
    for (i = 0; i < len; i++) {
        unsigned char low = vals[2*i];
        unsigned char high = vals[(2*i)+1];
        
        signed short value = (high << 8) | low;
        
        data[i] = value;
    }
}