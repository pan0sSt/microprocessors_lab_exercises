#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include <header.h>

void FIQ_handler(void);

PIO* pioa = NULL;
AIC* aic = NULL;
TC* tc = NULL;

int main( int argc, const char* argv[] ){
	char tmp;
	unsigned int i, counter, bitmask, next;

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

	counter = 100;

	while( (tmp = getchar()) != 'e')
	{
		counter += 1;
		if(counter >= 100)
		{
			pioa -> CODR = 0x7F; //ΚΑΘΑΡΙΣΜΟΣ ΟΛΩΝ ΤΩΝ OUTPUTS(LEDS)
			bitmask = 1;
			next = 20;
			counter = 0;
		}else{
			if(counter >= next && counter <= 80)
			{
				pioa -> SODR = bitmask; //ΕΝΕΡΓΟΠΟΙΗΣΗ OUTPUT(LED)
				bitmask <<= 1;
				next += 10;
			}
		}
	}

	CLEANUP;

	return 0;
}

void FIQ_handler(void){}