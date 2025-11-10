/*
 * uart.c
 *
 * Created: 22.8.2024 14.27.00
 * Author : Jessé Forest
 */

#include "uart.h"

uint16_t txData;                                    // UART internal variable for TX
//uint8_t rxBuffer;                                 // Incoming UART character buffer

//------------------------------------------------------------------------------
// Function configures Timer_A for full-duplex UART operation
//------------------------------------------------------------------------------
void TimerA_UART_init(void)
{
    TACCTL0 = OUT;                                  // Set TXD Idle as Mark = '1'
    TACCTL1 = SCS + CM1 + CAP + CCIE;               // Sync, Neg Edge, Capture, Int
    TACTL = TASSEL_2 + MC_2;                        // SMCLK, start in continuous mode
}

//------------------------------------------------------------------------------
// Function halts Timer_A
//------------------------------------------------------------------------------
void TimerA_UART_halt(void)
{
    TACTL &= ~(MC_2);                               // SMCLK, stop
}

//------------------------------------------------------------------------------
// Outputs one byte using the Timer_A UART
//------------------------------------------------------------------------------
void TimerA_UART_tx(unsigned char byte)
{
    while (TACCTL0 & CCIE);                         // Ensure last char got TX'd
    TACCR0 = TAR;                                   // Current state of TA counter
    TACCR0 += UART_TBIT;                            // One bit time till first bit
    TACCTL0 = OUTMOD0 + CCIE;                       // Set TXD on EQU0, Int
    txData = byte;                                  // Load global variable
    txData |= 0x100;                                // Add mark stop bit to TXData
    txData <<= 1;                                   // Add space start bit
}

//------------------------------------------------------------------------------
// Prints a string over using the Timer_A UART
//------------------------------------------------------------------------------
void TimerA_UART_print(char *string)
{
    while (*string) {
        TimerA_UART_tx(*string++);
    }
}

#pragma vector = WDT_VECTOR
__interrupt void watchdog_timer (void)
{
  __bic_SR_register_on_exit(LPM3_bits);             // Clear LPM3 bits from 0(SR)
}

#pragma vector = TIMERA0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    static unsigned char txBitCnt = 10;

    TACCR0 += UART_TBIT;                            // Add Offset to CCRx
    if (txBitCnt == 0) {                            // All bits TXed?
        TACCTL0 &= ~CCIE;                           // All bits TXed, disable interrupt
        txBitCnt = 10;                              // Re-load bit counter
    }
    else {
        if (txData & 0x01) {
          TACCTL0 &= ~OUTMOD2;                      // TX Mark '1'
        }
        else {
          TACCTL0 |= OUTMOD2;                       // TX Space '0'
        }
        txData >>= 1;
        txBitCnt--;
    }
}
