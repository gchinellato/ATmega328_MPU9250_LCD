/**
*************************************************
* @Project: Self Balance
* @Platform: Arduino Nano ATmega328
* @Description: Main thread
* @Owner: Guilherme Chinellato
* @Email: guilhermechinellato@gmail.com
*************************************************
*/

#ifndef MAIN_H
#define MAIN_H

#define SERIAL_BAUDRATE 	38400
#define DATA_INTERVAL 	    20 // ms
#define DATA_INTERVAL_LCD 	1100 // ms

/* GPIO mapping */
#define PWM1_PIN 9
#define CW1_PIN 7
#define CCW1_PIN 8

#define PWM2_PIN 10
#define CW2_PIN 5
#define CCW2_PIN 6

float dt=0; // duration time
unsigned int timestamp;
unsigned int timestamp_old;
float *ori; // orientation vector (roll, pitch, yaw)

#endif
