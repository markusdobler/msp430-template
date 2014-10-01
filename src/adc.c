#include <msp430.h>
#include <legacymsp430.h>

#include <msp430x22x2.h>
void main(void){
  WDTCTL = WDTPW + WDTHOLD;
  DCOCTL = CALDCO_16MHZ;
  BCSCTL1 = CALBC1_16MHZ;
  ADC10CTL1 |= CONSEQ1; //continuous sample mode, MUST BE SET FIRST!
  ADC10CTL0 |= ADC10SHT_2 + ADC10ON + MSC; //sample and hold time, adc on, cont. sample
  ADC10AE0 |= 0x01; // select channel A0
  ADC10CTL0 |= ADC10SC + ENC; // start conversions

  #define LED BIT6

  P1DIR |= LED;

  while(1){
    if(ADC10MEM>630) {
      P1OUT |= LED;
    } else {
      P1OUT &= ~LED;
    }
  }
}
