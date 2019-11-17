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
#define LED_LEFT 1
#define LED_RIGHT 2

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
	unsigned int gen, i, counter, bitmask, next;

	STARTUP; //ΑΡΧΙΚΟΠΟΙΗΣΗ ΣΥΣΤΗΜΑΤΟΣ

	pioa -> PER = 0;
	pioa -> OER = 0;
	pioa -> CODR = 0;
	for( i=0; i<7; i++ ) 
	{
		pioa -> PER |= (1)<<(i) ; //ΓΡΑΜΜΗ i: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
		pioa -> CODR |= (1)<<(i) ; //ΓΡΑΜΜΗ i: ΔΥΝΑΜΙΚΟ ΕΞΟΔΟΥ LOW
		pioa -> OER |= (1)<<(i) ; //ΓΡΑΜΜΗ i: ΛΕΙΤΟΥΡΓΙΑ ΕΞΟΔΟΥ
	}

	tc->Channel_0.RC  = 1638; //8192(period of 1 sec) * 0.2(5Hz = 0.2 sec) = 1638
	tc->Channel_0.CMR = 0x2084; //SLOW CLOCK , WAVEFORM , DISABLE CLK ON RC COMPARE
	tc->Channel_0.IDR = 0xFF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ ΔΙΑΚΟΠΩΝ
	tc->Channel_0.IER = 0x10; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΜΟΝΟ ΤΟΥ RC COMPARE

	gen       = tc->Channel_0.SR; //TC0 : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	pioa->PER |= (1)<<(7); //ΓΡΑΜΜΗ 7: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
	pioa->PUER = (1)<<(7); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΣΤΗ ΓΡΑΜΜΗ 7: PULL−UP
	pioa->ODR = (1)<<(7); //ΓΡΑΜΜΗ 7: ΛΕΙΤΟΥΡΓΙΑ ΕΙΣΟΔΟΥ

	gen       = pioa->ISR; // PIOA: ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	pioa->IER = (1)<<(7); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΔΙΑΚΟΠΩΝ ΣΤΗ ΓΡΑΜΜΗ 7

	aic->FFER = (1<<PIOA_ID) | (1<<TC0_ID); //ΟΙ ΔΙΑΚΟΠΕΣ 2 ,17 ΕΙΝΑΙ ΤΥΠΟΥ FIQ
	aic->IECR = (1<<PIOA_ID) | (1<<TC0_ID); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΔΙΑΚΟΠΩΝ : PIOA & TC0
	aic->ICCR = (1<<PIOA_ID)|(1<<TC0_ID); //AIC : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	counter = 100;
	mask = 1;

	while( (tmp = getchar()) != 'e')
	{
		counter += 1;
		if(counter >= 100)
		{
			pioa -> CODR = 0x7F; //ΚΑΘΑΡΙΣΜΟΣ ΟΛΩΝ ΤΩΝ OUTPUTS(LEDS)
			bitmask = mask;
			next = 20;
			counter = 0;
		}else{
			if(counter >= next && counter <= 80)
			{
				pioa -> SODR = bitmask; //ΕΝΕΡΓΟΠΟΙΗΣΗ OUTPUT(LED)
				bitmask <<= 1;
				if(bitmask == 0x80) bitmask = 1;
				next += 10;
			}
		}
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

	fiq = aic->IPR; //ΕΝΤΟΠΙΣΜΟΣ ΠΕΡΙΦΕΡΕΙΑΚΟΥ ΠΟΥ ΠΡΟΚΑΛΕΣΕ ΤΗ ΔΙΑΚΟΠΗ
	
	if( fiq & (1<<PIOA_ID) ) //ΕΛΕΓΧΟΣ ΓΙΑ PIOA
	{
		data_in = pioa->ISR; //ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΠΗΓΗΣ ΤΗΣ ΔΙΑΚΟΠΗΣ
		aic->ICCR = (1<<PIOA_ID); //ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΔΙΑΚΟΠΗΣ ΑΠΟ AIC
		data_in = pioa->PDSR; //ΑΝΑΓΝΩΣΗ ΤΙΜΩΝ ΕΙΣΟΔΟΥ
		if( data_in & 0x80 ) //ΔΙΑΚΟΠΤΗΣ ΠΑΤΗΜΕΝΟΣ
		{
			if(button_state == BUT_IDLE)
			{
				button_state = BUT_PRESSED;
				if( led_state == LED_IDLE ) //ΑΝ ΔΕΝ ΑΝΑΒΟΣΒΗΝΕΙ
				{
					led_state = LED_RIGHT;
					tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ					
				}else if( led_state == LED_RIGHT )
				{
					led_state = LED_LEFT;
					tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ		
				}else{
					led_state = LED_IDLE;
					tc->Channel_0.CCR = 0x02; //ΔΙΑΚΟΠΗ ΜΕΤΡΗΤΗ
				}
			}
		}else{
			if(button_state == BUT_PRESSED) button_state = BUT_IDLE;
		}
	}
	
	if( fiq & (1<<TC0_ID) )
	{
		data_out = tc->Channel_0.SR;//ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΠΗΓΗΣ ΤΗΣ ΔΙΑΚΟΠΗΣ
		aic->ICCR = (1<<TC0_ID); //ΕΚΚΑΘΑΡΙΣΗ ΔΙΑΚΟΠΗΣ ΚΑΙ ΑΠΟ AIC
		if(led_state == LED_RIGHT)
		{
			mask >>= 1;
			if(mask == 0x00) mask = 0x40;
		}else if( led_state == LED_LEFT )
		{
			mask <<= 1;
			if(mask == 0x80) mask = 1;
		}

		tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ
	}
}