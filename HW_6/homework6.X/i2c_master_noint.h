#ifndef I2C_MASTER_NOINT_H__
#define I2C_MASTER_NOINT_H__

#include <xc.h>

void i2c_master_setup(void);                    // set up I2C1 as master
void i2c_master_start(void);                    // send a START signal
void i2c_master_restart(void);                  // send a RESTART signal
void i2c_master_send(unsigned char byte);       // send a byte (address/data)
unsigned char i2c_master_recv(void);            // receive data byte
void i2c_master_ack(int val);                   // send ACK (0) or NACK (1)
void i2c_master_stop(void);                     // send STOP signal

void i2c_master_read_multiple(unsigned char reg, unsigned char add_w, unsigned char add_r, unsigned char * data, int length);

#endif