//MSP_Tinaco4 - Bomba5

#include <msp430.h>
#include <stdio.h>
#include <string.h>
int i,dist=0,conta=0,seg=0,cuarto=0,ctrRecep=0;
char dato;
char str[20]={};
char str2[20]={};
long temp=0;
int IntDegC=0, check=0;

void envio_status(void);
void calibrar_osc(void);
void config_uart(void);
void lecturaInic(void);
void calcTemp(void);
void envioCadena(char[]);
void borrarCadena(void);

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;
  calibrar_osc();
  config_uart();

  P1DIR |= BIT3 + BIT6;          //Salidas P1.6 (LED Verde) y P1.3 (Trigger)
  P1DIR &= ~BIT4;               // P1.4 entrada (Echo)
  P1IES &= ~BIT4;               // P1.4 Configurar tipo de interrupci�n: de bajo a alto.
  P1IE |=  BIT4;                // P1.4 Interrupci�n habilitada

  TACCR0 = 50000;
  TA0CTL = TASSEL_2 + MC_1;                  // SMCLK, va contar de 0 hacia arriba
  TACCTL0 = CCIE;                             // CCR0 se habilita interrupci�n

  ADC10CTL1 = INCH_10 + ADC10DIV_3;         // Temp Sensor ADC10CLK/4
  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON;

  for(i=0;i<3;i++){ //3 parpadeos de inicio
     P1OUT |= BIT6;
     __delay_cycles(1000000);
     P1OUT &= ~BIT6;
     __delay_cycles(1000000);
  }

  __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled (se hace dormir al procesador) Ahorro de energ�a
}

//Interrupci�n del timer
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
   /*seg++;
   if( seg >= 20*900 ){
      cuarto++;
      if( cuarto == 24 )
          WDTCTL &= ~(WDTPW + WDTHOLD);        // Resetear cada 6 hrs
   }//*/

}

//Interrupci�n de RX
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    while (!(IFG2&UCA0TXIFG));
    dato = UCA0RXBUF;

    if(dato=='s'){                                  //Verificar si es solicitud de MSP_Bomba
        for(i=0;i<2;i++){                               //Recepci�n de se�al por UART, 2 pips
           P1OUT |= BIT6;
           __delay_cycles(100000);
           P1OUT &= ~BIT6;
           __delay_cycles(100000);
        }

        P1OUT |= BIT3;                              // Iniciar verificaci�n de nivel de agua
        __delay_cycles(10);
        P1OUT &= ~BIT3;
    }
}
//Interrupci�n de puerto (P1.4) Echo del Sensor ultras�nico
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
  while(P1IN&BIT4){
      conta++;
  }
  dist = ((conta*100)/58)/10;                    // Conversi�n de tiempo a distancia
  conta=0;
  P1IFG &= ~BIT4;

  envio_status();                           // Metodo para preparaci�n de envio de respuesta
}

void envio_status(void){        //Construcci�n de respuesta (i=inicio trama) (*=fin trama en tapa) (#=fin trama en bomba)
    calcTemp();                             //C�lculo de temperatuta interna

    check=dist+IntDegC;
    if(dist > 50){                         // Si la distancia es mayor a 50 cm, el tinaco necesita agua
        P1OUT |= BIT6;
        __delay_cycles(1000000);
        P1OUT &= ~BIT6;
        __delay_cycles(1000000);
    }

    sprintf(str, "i%d$%d$%d$#*", dist, IntDegC, check); // Cadena status tapa

    envioCadena(str);   //M�todo para enviar la cadena construida por la UART
    //borrarCadena();     //M�todo para limpiar el espacio de memoria despu�s de enviar la cadena
}

void borrarCadena(void){
    int z;
    for(z = 0; z<21; z++){
        str[z]=' ';
    }
}

void calcTemp(void){
    ADC10CTL0 |= ENC + ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);
    temp = ADC10MEM;                        //se guarda la temperatura en temp
    IntDegC = ((temp - 673) * 423) / 1024;  //temp se convierte a �C
}

void envioCadena(char cadena[20]){
    char k=0, y=0;
    while(y==0){
        if( !(cadena[k]=='*') ){             //si la cadena no ha llegado al final -> *
            while (!(IFG2&UCA0TXIFG));      //se transmiten los datos
            UCA0TXBUF = cadena[k];
        }else {y=1;}
        k++;
    }
}

void config_uart(void){
        P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
        P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD
        UCA0CTL1 |= UCSSEL_2;                     // SMCLK
        UCA0BR0 = 104;                            // 1MHz 9600 (1 seg/9600 = 104 microSeg)
        UCA0BR1 = 0;                              // 1MHz 9600
        UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
        UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
        IE2 |= UCA0RXIE;                          // Habilito interrupci�n de RX.
}

void calibrar_osc(void){
    if (CALBC1_1MHZ==0xFF)                    // If calibration constant erased
        {
            while(1);                               // do not load, trap CPU!!
        }
        DCOCTL = 0;                               // Select lowest DCOx and MODx settings
        BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
        DCOCTL = CALDCO_1MHZ;
}
