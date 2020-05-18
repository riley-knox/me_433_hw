/*
 * I2C Master utilities using polling (not interrupts)
 * 
 * Call functions in correct order per I2C protocol
 * 
 * Use pull-up resistors with I2C pins (2-10K)
*/

#include "i2c_master_noint.h"

void i2c_master_setup(void) {
    // using a large BRG to see it on the nScope, make it smaller after verifying that code works
    // look up TPGD in the datasheet
    
    I2C1BRG = 40;         // I2CBRG = [1/(2*Fsck) - TPGD]*Pblck - 2 (TPGD is the Pulse Gobbler Delay)
    I2C1CONbits.ON = 1;     // turn on the I2C1 module
}

void i2c_master_start(void) {
    I2C1CONbits.SEN = 1;            // send the start bit
    
    while (I2C1CONbits.SEN) {;}     // wait for the start bit to be sent
}

void i2c_master_restart(void) {
    I2C1CONbits.RSEN = 1;           // send restart bit
    
    while (I2C1CONbits.RSEN) {;}    // wait for restart to clear
}

void i2c_master_send(unsigned char byte) {  // send byte to slave
    I2C1TRN = byte;
    
    while (I2C1STATbits.TRSTAT) {;}         // wait for transmission to finish
    
    if (I2C1STATbits.ACKSTAT) {             // high if no acknowledgment from slave
        while(1) {;}                        // get stuck here if no acknowledgment
    }
}

unsigned char i2c_master_recv(void) {   // receive byte from slave
    I2C1CONbits.RCEN = 1;               // start receiving data
    
    while (!I2C1STATbits.RBF) {;}       // wait to receive data
    
    return I2C1RCV;                     // read/return data
}

void i2c_master_ack(int val) {      // sends ACK (0, "send another byte") or NACK (1, "no more bytes")
    I2C1CONbits.ACKDT = val;        // store ACK/NACK in ACKDT
    
    I2C1CONbits.ACKEN = 1;          // send ACKDT
    
    while (I2C1CONbits.ACKEN) {;}   // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) {        // send STOP
    I2C1CONbits.PEN = 1;            // comms complete
    
    while (I2C1CONbits.PEN) {;}     // wait for STOP to complete
}

void i2c_master_read_multiple(unsigned char reg, unsigned char add_w, unsigned char add_r, unsigned char * data, int length){
    int i = 0;                  // counter
    
    i2c_master_start();
    i2c_master_send(add_w);
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(add_r);
    for (i = 0; i < length-1; i++){
        data[i] = i2c_master_recv();
        i2c_master_ack(0);
    }
    data[length-1] = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
}