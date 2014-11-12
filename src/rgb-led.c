#include <msp430.h>
#include <legacymsp430.h>
#include "hsl2rgb.h"

#ifndef NULL
#define NULL ( (void *) 0)
#endif

#define BUTTON BIT3

#define VAL_RED TA0CCR1
#define VAL_GREEN TA1CCR1
#define VAL_BLUE TA1CCR2

#define PWM_PERIOD 1024
int MAX_DURATION = PWM_PERIOD-1;


// Delay Routine from mspgcc help file
static void __inline__ delay(register unsigned int n)
{
  __asm__ __volatile__ (
  "1: \n"
  " dec %[n] \n"
  " jne 1b \n"
        : [n] "+r"(n));
}


void init_adc_continuous_mode(int channel, volatile unsigned int * adc10sa)
{
  /* Initialize ADC for continuous sampling
   *
   * Depending on adc10sa, either a single channel or a sequence of channels is used:
   * * for adc10sa==NULL, the channel specified by channel (e.g. INCH_1) is used;
   *   the adc result is stored in ADC10MEM.
   * * otherwise, all channels from INCH_0 to {channel} are used in sequence-of-channels mode;
   *   the adc results are stored in an array starting at address `adc10sa` (with the result from
   *   {channel} at `adc10sa[0]`, result from {channel}-1 at `adc10sa[1]`, ...)
   */
  ADC10CTL0 &= ~ENC;
  ADC10CTL0 = ADC10ON;
  while(ADC10CTL1 & ADC10BUSY) {
    // wait until adc stops
  }
  int ch_id = channel >> 12;
  if(adc10sa) {
    ADC10DTC0 = ADC10CT;
    ADC10DTC1 = ch_id + 1;
    ADC10AE0 = (1 << ch_id)*2 - 1; // enable all required channels
    ADC10SA = ((unsigned int)adc10sa);
  } else {
    ADC10DTC0 = 0;
    ADC10AE0 = 1 << ch_id; // enable single channel
  }

  ADC10CTL0 |= ADC10SHT_1 | SREF_0 | MSC;
  ADC10CTL1 |= channel + (adc10sa ? CONSEQ_3 : CONSEQ_2);

  ADC10CTL0 |= ENC;
  ADC10CTL0 |= ADC10SC;
}


void update_rgb_color_splines(int);

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD; // Stop WDT

  P1DIR |= BIT2; // P1.2 to output
  P2DIR |= BIT1 | BIT5; // P2.1 and P2.5 to output

  P1SEL |= BIT2; // P1.2 to TA0.1
  P2SEL |= BIT1 | BIT5; // P2.1 and P2.5 to TA1.1 and TA1.2

  BCSCTL1 &= (BCSCTL1&0xf0) | 0x07;    // RSEL = 7 -> DCOCLK ~ 1 MHz
  //BCSCTL1 &= ~0xf;    // RSEL = 0 -> DCOCLK ~ 100 kHz
  //BCSCTL2 |= DIVS_3;           // SMCLK = DCOCLK/8

  TA0CCR0 = TA1CCR0 = PWM_PERIOD+1; // PWM Period
  TA0CCTL1 = TA1CCTL1 = TA1CCTL2 = OUTMOD_7; // TAxCCRy reset/set
  TA0CTL = TA1CTL = TASSEL_2 | MC_1; // SMCLK, up mode

  VAL_RED   = PWM_PERIOD-1; // CCR1 PWM duty cycle
  VAL_GREEN = PWM_PERIOD-1; // CCR1 PWM duty cycle
  VAL_BLUE  = PWM_PERIOD-1; // CCR2 PWM duty cycle

  //// interrupt from high to low
  //P1IES |= BUTTON;
  //// clear the interrupt flag
  //P1IFG &= ~BUTTON;
  //// enable interrupt on BIT3
  //P1IE |= BUTTON;
  //
  //DCOCTL = CALDCO_16MHZ;

  volatile unsigned int adc_values[2];
  //init_adc_continuous_mode(INCH_1, adc_values);
  init_adc_continuous_mode(INCH_0, NULL);
#define LED BIT6
  P1DIR |= LED;

  update_rgb_color_splines(0);
  while (1) {
    delay(2000);
    if (P1IN & BIT5) {
      update_rgb_color_splines(1);
    } else {
    }
    if (P1IN & BIT3) {
    } else {
      //MAX_DURATION = adc_values[1];
      MAX_DURATION =  ADC10MEM;
      update_rgb_color_splines(0);
    }
    if(adc_values[0]&0x40) {
      P1OUT |= LED;
    } else {
      P1OUT &= ~LED;
    }
  }

  _BIS_SR(GIE); // enable interrupts
  _BIS_SR(LPM0_bits); // Enter LPM0
}


interrupt(PORT1_VECTOR) p1_isr(void) {
  // check if BIT3 button has set flag
  switch (P1IFG & BUTTON) {
    case BUTTON:
      P1IFG = P1IFG & ~BUTTON;
      update_rgb_color_splines(1);
      _BIS_SR(GIE); // enable interrupts
      _BIS_SR(LPM0_bits); // Enter LPM0
      break;
    default:
      break;
  }
  return;
}

void update_rgb_color_rgb_test(void) {
  static int col = 0;
  col++;
  VAL_RED   = (col&1) ? (PWM_PERIOD-1) : 0;
  VAL_GREEN = (col&2) ? (PWM_PERIOD-1) : 0;
  VAL_BLUE  = (col&4) ? (PWM_PERIOD-1) : 0;
}


void update_rgb_color_splines(int inc) {
  static unsigned long cnt = 0;
  const unsigned char points[][3] = {
    {220, 1, 20},
    {220, 20, 1},
    {110, 110, 1},
    {1, 200, 1},
    {1, 100, 130},
    {1, 1, 250},
    {100, 1, 130},
  };
  const unsigned long P = sizeof(points)/sizeof(points[0]);
  const unsigned long steps = 40;

  cnt = (cnt+inc) % (P*steps);
  unsigned long pa = cnt / steps;
  unsigned long pb = (pa+1) % P;

  unsigned long mb = cnt % steps;
  unsigned long ma = steps - mb;

  VAL_RED   = (ma*points[pa][0] + mb*points[pb][0]) * (unsigned long)(MAX_DURATION) / (steps*255);
  VAL_GREEN = (ma*points[pa][1] + mb*points[pb][1]) * (unsigned long)(MAX_DURATION) / (steps*255);
  VAL_BLUE  = (ma*points[pa][2] + mb*points[pb][2]) * (unsigned long)(MAX_DURATION) / (steps*255);
}


// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
	__bic_SR_register_on_exit(CPUOFF);        // Return to active mode }
}
