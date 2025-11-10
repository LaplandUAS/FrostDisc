/*
 * uart.h
 *
 * Created: 22.8.2024 14.27.00
 * Author : Jessé Forest
 */

#ifndef UART_H_
#define UART_H_

#include <msp430.h>
#include <stdint.h>

#define UART_TXD            0x02                    // TXD on P1.1 (Timer0_A.OUT0)
//#define UART_RXD            0x04                    // RXD on P1.2 (Timer0_A.CCI1A)

#define UART_TBIT_DIV_2     (1000000 / (9600 * 2))  // Baudrate clock divider
#define UART_TBIT           (1000000 / 9600)        // Baudrate

void TimerA_UART_init(void);
void TimerA_UART_halt(void);
void TimerA_UART_tx(unsigned char byte);
void TimerA_UART_print(char *string);
#endif /* UART_H_ */
