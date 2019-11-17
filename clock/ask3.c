#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include <header.h>

#define PIOA_ID 2
#define TC0_ID 17

#define BUT_IDLE 0
#define BUT_PRESSED 1
#define BUT_RELEASED 2

#define LED_IDLE 0
#define LED_ON 1
#define LED_HOLD 2

void FIQ_handler(void);

PIO* pioa = NULL;
AIC* aic = NULL;
TC* tc = NULL;

unsigned int button_state = BUT_IDLE;
unsigned int led_state = LED_IDLE;
int mask;

int main( int argc, const char* argv[] )
{
	char tmp;
	unsigned int gen, i;

	STARTUP; //ΑΡΧΙΚΟΠΟΙΗΣΗ ΣΥΣΤΗΜΑΤΟΣ

	pioa -> PER = 0;
	pioa -> OER = 0;
	pioa -> CODR = 0;
	for( i=0; i<8; i++ ) 
	{
		pioa -> PER |= (1)<<(i) ; //ΓΡΑΜΜΗ i: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
		pioa -> CODR |= (1)<<(i) ; //ΓΡΑΜΜΗ i: ΔΥΝΑΜΙΚΟ ΕΞΟΔΟΥ LOW
		pioa -> OER |= (1)<<(i) ; //ΓΡΑΜΜΗ i: ΛΕΙΤΟΥΡΓΙΑ ΕΞΟΔΟΥ
	}

	tc->Channel_0.RC  = 8192; //8192(period of 1 sec) ~ 5Hz
	tc->Channel_0.CMR = 0x2084; //SLOW CLOCK , WAVEFORM , DISABLE CLK ON RC COMPARE
	tc->Channel_0.IDR = 0xFF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ ΔΙΑΚΟΠΩΝ
	tc->Channel_0.IER = 0x10; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΜΟΝΟ ΤΟΥ RC COMPARE

	gen       = tc->Channel_0.SR; //TC0 : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	pioa->PER |= (1)<<(9); //ΓΡΑΜΜΗ 9: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
	pioa->PUER = (1)<<(9); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΣΤΗ ΓΡΑΜΜΗ 9: PULL−UP
	pioa->ODR = (1)<<(9); //ΓΡΑΜΜΗ 9: ΛΕΙΤΟΥΡΓΙΑ ΕΙΣΟΔΟΥ

	gen       = pioa->ISR; // PIOA: ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	pioa->IER = (1)<<(9); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΔΙΑΚΟΠΩΝ ΣΤΗ ΓΡΑΜΜΗ 9

	aic->FFER = (1<<PIOA_ID) | (1<<TC0_ID); //ΟΙ ΔΙΑΚΟΠΕΣ 2 ,17 ΕΙΝΑΙ ΤΥΠΟΥ FIQ
	aic->IECR = (1<<PIOA_ID) | (1<<TC0_ID); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΔΙΑΚΟΠΩΝ : PIOA & TC0
	aic->ICCR = (1<<PIOA_ID) | (1<<TC0_ID); //AIC : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ



	while( (tmp = getchar()) != 'e')
	{
		
	}

	aic->IDCR = (1<<PIOA_ID) | (1<<TC0_ID); //ΔΙΑΚΟΠΗ ΤΩΝ AIC interrupts
	tc->Channel_0.CCR = 0x02; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΤΟΥ Timer

	CLEANUP;

	return 0;
}

void FIQ_handler(void)
{
	unsigned int data_in = 0;
	unsigned int fiq = 0;
	unsigned int data_out;
	unsigned int reset = 0;

	fiq = aic->IPR; //ΕΝΤΟΠΙΣΜΟΣ ΠΕΡΙΦΕΡΕΙΑΚΟΥ ΠΟΥ ΠΡΟΚΑΛΕΣΕ ΤΗ ΔΙΑΚΟΠΗ
	
	if( fiq & (1<<PIOA_ID) ) //ΕΛΕΓΧΟΣ ΓΙΑ PIOA
	{
		data_in = pioa->ISR; //ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΠΗΓΗΣ ΤΗΣ ΔΙΑΚΟΠΗΣ
		aic->ICCR = (1<<PIOA_ID); //ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΔΙΑΚΟΠΗΣ ΑΠΟ AIC
		data_in = pioa->PDSR; //ΑΝΑΓΝΩΣΗ ΤΙΜΩΝ ΕΙΣΟΔΟΥ
		
		if( data_in & 0x200 ) //ΔΙΑΚΟΠΤΗΣ ΠΑΤΗΜΕΝΟΣ(0010 0000 0000(b))
		{
			if(button_state == BUT_RELEASED)
			{
				button_state = BUT_PRESSED;
				if( led_state == LED_IDLE ) //ΑΝ ΔΕΝ ΑΝΑΒΟΣΒΗΝΕΙ
				{
					led_state = LED_ON;
					tc->Channel_0.CCR = 0x02; //ΔΙΑΚΟΠΗ ΜΕΤΡΗΤΗ
					data_in = tc->Channel_0.SR; //TC0 : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ
					tc->Channel_0.RC  = 8192; //8192(period of 1 sec) ~ 5Hz
					tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ					
				}else if( led_state == LED_ON )
				{
					led_state = LED_HOLD;
					tc->Channel_0.CCR = 0x02; //ΔΙΑΚΟΠΗ ΜΕΤΡΗΤΗ
					data_in = tc->Channel_0.SR; //TC0 : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ
					tc->Channel_0.RC  = 8192/2; //8192/2(period of 0.5 sec) ~ 2Hz
					tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ		
				}else{
					led_state = LED_IDLE;
					tc->Channel_0.CCR = 0x02; //ΔΙΑΚΟΠΗ ΜΕΤΡΗΤΗ
					pioa->CODR = 0x100 & 0x200; //(0001 0000 0000 & 0010 0000 0000)(b)
				}
			}
		}else{
			if(button_state == BUT_PRESSED)
			{
				button_state = BUT_RELEASED;
				reset = 0;
			}
		}
	}
	
	if( fiq & (1<<TC0_ID) )
	{
		data_out = tc->Channel_0.SR;//ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΠΗΓΗΣ ΤΗΣ ΔΙΑΚΟΠΗΣ
		aic->ICCR = (1<<TC0_ID); //ΕΚΚΑΘΑΡΙΣΗ ΔΙΑΚΟΠΗΣ ΚΑΙ ΑΠΟ AIC
		if(led_state == LED_ON)
		{
			counter++;
			if (counter > 59) counter = 0;
           	pioa->SODR = counter%10<<4 | counter/10;
           	if (button_state == BUT_PRESSED) reset += 2;
		}else if( led_state == LED_HOLD )
		{
			if(pioa->ODSR & 0x100) pioa->CODR |= 0x100;
            else pioa->SODR |= 0x100;

            if (button_state == BUT_PRESSED) reset += 1;
		}

		if(reset > 1) {
			counter = 0;
         	pioa->SODR = counter%10<<4 | counter/10;

            reset = 0;
            button_state = BUT_IDLE;
            button_state = BUT_RELEASED;
		}

		tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ
	}
}