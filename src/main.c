/*
 * main.c
 *
 * Created: 5.9.2023 12.28.35
 * Author : Jessé Forest
 */
 
//P1.0- General peripheral power pin(enabled on timer)
//P1.1- [RESERVED] for radio UART
//P1.2- Network status pin from radio(read on radio wakeup)
//P1.3- [RESERVED] ADC pin
//P1.4- Radio wakeup pin(enabled when radio needs to wake up)

#include <msp430g2231.h>
#include "usi_i2c.h"
#include "toHex.h"
#include "uart.h"

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define ERROR_MARGIN        0x10                    // How many tries before giving up during various routines
#define MAX_ISR             0x1A                    // Counter limit (~20sec/inc) (1A is ~10min)
#define UARTCMD             "at+sendb="             // Serial message string
#define TERMINATION         "\r"                    // Serial termination string
#define SLEEPCMD            "at+sleep\r\0"          // Sleep command
#define JOINSTATUS          "at+njs\r\0"            // Join status
#define SENSORS             0x04                    // Number of connected sensors
#define TRIGGER_TEMP        4095                    // Temperature threshold (1280 for +10c, 4095 for +31c)

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
const uint8_t tmp[] =                               // Sensor i2c address table (shifted left)
{
 0x90,                                              // 0x48
 0x92,                                              // 0x49
 0x94,                                              // 0x4A
 0x96                                               // 0x4B
};

uint16_t cfg[] =                                    // i2c configuration payload
{
 0x90,                                              // Device address (Doesn't matter here. Replaced procedurally)
 0x01,                                              // Command (Configure mode)
 0x0C,                                              // Command (Mode MSB)
 0x00                                               // Command (Mode LSB)
};

uint16_t pld[] =                                    // i2c temperature reading payload
{
 0x90,                                              // Device address (Doesn't matter here. Replaced procedurally)
 0x00,                                              // Command (Retrieve temperature)
 I2C_RESTART,                                       // Followed by i2c-restart
 0x91,                                              // Device address shifted by 1 (Doesn't matter here. Replaced procedurally)
 I2C_READ,                                          // Read MSB
 I2C_READ                                           // Read LSB
};

char status[5];                                     // Status buffer
uint8_t sbt = 0x00;                                 // Status byte
uint8_t i;                                          // Iterator
uint8_t isrC = (MAX_ISR-1);                         // Interrupt counter, make sure this is only a single cycle on first boot!
uint16_t rdx[SENSORS];                              // Sensor readings

//------------------------------------------------------------------------------
// Function prototypes
//------------------------------------------------------------------------------
void AdcInit(void);
void AdcStart(void);
void AdcDisable(void);
int GetAdcValue(void);

//------------------------------------------------------------------------------
// main()
//------------------------------------------------------------------------------
int main(void)
{
  BCSCTL1 = CALBC1_1MHZ;                            // Base clock to 1MHz
  BCSCTL1 |= DIVA_3;                                // ACLK/2
  BCSCTL3 |= LFXT1S_2;                              // ACLK = VLO

  WDTCTL = WDT_ADLY_1000;                           // Interval timer
  DCOCTL = CALDCO_1MHZ;                             // Calibrate clock

  IE1 |= WDTIE;                                     // Enable WDT interrupt

  P1OUT = 0xC0;                                     // All P1.x reset, P1.6 & P1.7 & P1.0 to out
  P1REN |= 0xC0;                                    // P1.6 & P1.7 Pullups
  P1SEL = UART_TXD;                                 // Timer function for TXD pin (P1.1)
  P1DIR = 0xFB;                                     // Set all pins to output on port 1
  P2OUT = 0x00;                                     // All P2.x reset
  P2SEL = 0x00;                                     // Don't configure anything (for now)
  P2DIR = 0xFF;                                     // Set all pins to output on port 2
  AdcInit();                                        // Initialize ADC

  __enable_interrupt();                             // Enable ISR
  while(1)                                          // Main loop
  {
    if(isrC++ == MAX_ISR)                           // Check Interrupt counter and increment
    {
        P1OUT |= BIT0;                              // Enable P1.0
        AdcStart();                                 // Start ADC conversion
        i2c_init(USIDIV_5, USISSEL_2);              // i2c USI registers
        __delay_cycles(1500);                       // Time for i2c to start...
        for(i=SENSORS; i-->0;)                      // Iterate through i2c devices
        {
            cfg[0] = tmp[i];                        // Set i2c slave address
            i2c_send_sequence(cfg, 4, 0 ,LPM3_bits);// Transmit payload over i2c
            while(!i2c_done());                     // Maybe more? 1500?
        }

        __delay_cycles(1500000);                    // Wait before writing to UART (The more you wait here the more accurate the i2c temperature reading becomes)
        for(i=SENSORS; i-->0;)
        {
            uint8_t b[2];                           // Response buffer
            pld[0] = tmp[i];                        // Set slave address
            pld[3] = tmp[i] + 1;                    // Set slave read address
            i2c_send_sequence(pld, 6, b, LPM3_bits);// Transmit payload over i2c
            while(!i2c_done());                     // Wait for bytes from slave
            __delay_cycles(1500);                   // Wait before writing to UART
            rdx[i]=((uint16_t)b[0]<<8)|b[1];        // Turn bytes to int and place in sensor reading array
            sbt = ((int16_t)rdx[i] <= TRIGGER_TEMP);// Threshold check

        }
        if(sbt)
        {
            P1OUT |= BIT4;                          // Enable wakeup pin
            TimerA_UART_init();                     // Start Timer_A UART
            i = ERROR_MARGIN;                       // Assign error margin to iterator
            do{
                TimerA_UART_print(JOINSTATUS);      // Transmit serial message. Try to wake up and update join status...
                __delay_cycles(80000);              // Waste some time trying
            }while(!(P1IN & BIT2));                 // Wait for NJS pin (or until tries run out)|| (i--) > 0
            TimerA_UART_print(UARTCMD);             // Transmit serial message
            for (i=SENSORS; i-->0;)                 // Iterate through every sensor
            {
                itohexa(status, rdx[i]);            // Write the 16-bit int to a buffer in HEX as ASCII
                //status[5] = '\0';                 // Ensure string termination with nullchar
                TimerA_UART_print(status);          // Add buffer to UART buffer
            }
            P1OUT &= ~BIT4;                         // Disable wakeup pin
            itohexa(status, GetAdcValue());         // Wait for ADC and read core voltage
            TimerA_UART_print(status);              // Write core voltage to UART
            TimerA_UART_print(TERMINATION);         // Transmit serial message
            do{
            TimerA_UART_print(SLEEPCMD);            // Put the radio to sleep shhh....
            __delay_cycles(80000);                  // Waste some time trying
            }while(P1IN & BIT2);                    // Wait for NJS pin (or until tries run out)|| (i--) > 0
            TimerA_UART_halt();                     // Halt TimerA UART
        }
        sbt = 0x00;                                 // Reset threshold check
        isrC = 0x00;                                // Reset interrupt counter
        P1OUT &= ~BIT0;                             // Disable P1.0
    }
    __bis_SR_register(LPM3_bits + GIE);             // Enter LPM3
  }
}

void AdcInit(void)
{
    ADC10AE0 = BIT5;                                // Set sampling pin
    ADC10CTL0 = SREF_1 + ADC10SHT_3;                // Enable ADC-core, 64 cycles sample time.
    ADC10CTL1 = INCH_5 + ADC10SSEL_2 + CONSEQ_0;    // Initialize ADC channel, clock source MCLK (At 1MHz), single sample
}

void AdcStart(void)
{
    ADC10CTL0 |= REFON + ADC10ON;                   // Turn on ADC and reference
    __delay_cycles(1000);                           // Give time for ADC to stabilize
    ADC10CTL0 |= ENC + ADC10SC;                     // Start ADC conversion
}

void AdcDisable(void)
{
    ADC10CTL0 &= ~ENC;                              // Stop conversion
    ADC10CTL0 &= ~(ADC10ON + REFON);                // Turn off ADC core and reference to save power
}

int GetAdcValue(void)
{
    while( ! (ADC10CTL0 & ADC10IFG));               // Wait for conversion ready flag
    ADC10CTL0 &= ~ADC10IFG;                         // Clear ready flag
    AdcDisable();                                   // Disable ADC core
    return ADC10MEM;                                // Return ADC reading
}
