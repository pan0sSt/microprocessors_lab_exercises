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

#define BUT_PRESSED 0
#define BUT_RELEASED 1

void FIQ_handler(void);

PIO* pioa = NULL;
AIC* aic = NULL;
TC* tc = NULL;

unsigned int button_start_state = BUT_RELEASED;
// unsigned int button_stop_state = BUT_RELEASED;
unsigned int state = 0;
unsigned int request_service = 0;

int main( int argc, const char* argv[] )
{
	char tmp;
	unsigned int gen, i;

	STARTUP; //ΑΡΧΙΚΟΠΟΙΗΣΗ ΣΥΣΤΗΜΑΤΟΣ

	pioa -> PER = 0x07; //ΓΡΑΜΜΗ 1 ΕΩΣ 3: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
	pioa -> CODR = 0x07; //ΓΡΑΜΜΗ 1 ΕΩΣ 3: ΔΥΝΑΜΙΚΟ ΕΞΟΔΟΥ LOW
	pioa -> OER = 0x07; //ΓΡΑΜΜΗ 1 ΕΩΣ 3: ΛΕΙΤΟΥΡΓΙΑ ΕΞΟΔΟΥ

	tc->Channel_0.RC  = 2048; //8192(period of 1 sec) * 0.25(1/4 sec) = 2048(4 Hz)
	tc->Channel_0.CMR = 0x2084; //SLOW CLOCK , WAVEFORM , DISABLE CLK ON RC COMPARE
	tc->Channel_0.IDR = 0xFF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ ΔΙΑΚΟΠΩΝ
	tc->Channel_0.IER = 0x10; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΜΟΝΟ ΤΟΥ RC COMPARE

	gen       = tc->Channel_0.SR; //TC0 : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	pioa->PER 		= 	0x08; //ΓΡΑΜΜΗ 4: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
	pioa -> PUER 	= 	0x08; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΣΤΗ ΓΡΑΜΜΗ 4: PULL−UP
	pioa -> ODR 	= 	0x08; //ΓΡΑΜΜΗ 4: ΛΕΙΤΟΥΡΓΙΑ ΕΙΣΟΔΟΥ

	gen       = pioa->ISR; // PIOA: ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	pioa -> IER 	= 	0x08; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΔΙΑΚΟΠΩΝ ΣΤΗ ΓΡΑΜΜΗ 4	

	aic->FFER = (1<<PIOA_ID) | (1<<TC0_ID); //ΟΙ ΔΙΑΚΟΠΕΣ 2 ,17 ΕΙΝΑΙ ΤΥΠΟΥ FIQ
	aic->IECR = (1<<PIOA_ID) | (1<<TC0_ID); //ΕΝΕΡΓΟΠΟΙΗΣΗ ΔΙΑΚΟΠΩΝ : PIOA & TC0
	aic->ICCR = (1<<PIOA_ID) | (1<<TC0_ID); //AIC : ΕΚΚΑΘΑΡΙΣΗ ΑΠΟ ΤΥΧΟΝ ΔΙΑΚΟΠΕΣ

	tc->Channel_0.CCR = 0x05; //ΕΝΑΡΞΗ ΜΕΤΡΗΤΗ

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
	unsigned int counter;

	fiq = aic->IPR; //ΕΝΤΟΠΙΣΜΟΣ ΠΕΡΙΦΕΡΕΙΑΚΟΥ ΠΟΥ ΠΡΟΚΑΛΕΣΕ ΤΗ ΔΙΑΚΟΠΗ
	
	if( fiq & (1<<PIOA_ID) ) //ΕΛΕΓΧΟΣ ΓΙΑ PIOA
	{
		data_in = pioa->ISR; //ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΠΗΓΗΣ ΤΗΣ ΔΙΑΚΟΠΗΣ
		aic->ICCR = (1<<PIOA_ID); //ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΔΙΑΚΟΠΗΣ ΑΠΟ AIC
		data_in = pioa->PDSR; //ΑΝΑΓΝΩΣΗ ΤΙΜΩΝ ΕΙΣΟΔΟΥ
		
		if( data_in & 0x08 ) //ΔΙΑΚΟΠΤΗΣ ΠΑΤΗΜΕΝΟΣ(1000(b))
		{
			if(button_start_state == BUT_RELEASED)
			{
		     	button_start_state = BUT_PRESSED;
		     	request_service = 1;
			}
		}else{
			if(button_start_state == BUT_PRESSED)
			{
				button_start_state = BUT_RELEASED;
			}
		}
	}

	
	if( fiq & (1<<TC0_ID) )
	{
		data_out = tc->Channel_0.SR;//ΕΚΚΑΘΑΡΙΣΗ ΤΗΣ ΠΗΓΗΣ ΤΗΣ ΔΙΑΚΟΠΗΣ
		aic->ICCR = (1<<TC0_ID); //ΕΚΚΑΘΑΡΙΣΗ ΔΙΑΚΟΠΗΣ ΚΑΙ ΑΠΟ AIC
		
		switch (state){
        case 0:
        	pioa->CODR = 0x02; //Φ ΚΟΚΚΙΝΟ
            pioa->SODR = 0x01; //Φ ΠΡΑΣΙΝΟ

        	state = (request_service == 1)?1:0;
        	counter = 0;
        	break;

        case 1:
        	pioa->CODR = 0x01; //Φ ΠΡΑΣΙΝΟ

        	if((counter % 2 == 0)) //ΕΛΕΓΧΟΣ ΑΡΤΙΟΥ COUNTER
			{
				pioa->SODR = 0x04; //Φ ΚΙΤΡΙΝΟ
			}else{
				pioa->CODR = 0x04; //Φ ΚΙΤΡΙΝΟ
			}

        	counter++;

        	if ( counter == 4*3 )
        	{
        		state = 2;
        		counter = 0;
        	}
        	break;

        case 2:
            pioa->CODR = 0x04; //Φ ΚΙΤΡΙΝΟ
        	pioa->SODR = 0x02; //Φ ΚΟΚΚΙΝΟ

        	counter++;
        	if (counter == 4*10)
        	{
        		state = 0;
        		request_service = 0;
        		counter = 0;
        	}
        	break;
		}
	}
}