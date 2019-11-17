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

void FIQ_handler(void);

int WhatDoesTheButtonSay(int state);

PIO* pioa = NULL;
AIC* aic = NULL;
TC* tc = NULL;

int main( int argc, const char* argv[] )
{
	char tmp;
	
int state = 0;
int flag = 0;
int i = 0;
int player1_score = 0;
int player2_score = 0;

	STARTUP; //ΑΡΧΙΚΟΠΟΙΗΣΗ ΣΥΣΤΗΜΑΤΟΣ

	pioa -> PER = 0x3FF; //ΓΡΑΜΜΗ 1 ΕΩΣ 10: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
	pioa -> CODR = 0x3FF; //ΓΡΑΜΜΗ 1 ΕΩΣ 10: ΔΥΝΑΜΙΚΟ ΕΞΟΔΟΥ LOW
	pioa -> OER = 0x3FF; //ΓΡΑΜΜΗ 1 ΕΩΣ 10: ΛΕΙΤΟΥΡΓΙΑ ΕΞΟΔΟΥ

	tc->Channel_0.RC  = 8192; //8192(period of 1 sec)
	tc->Channel_0.CMR = 0x2084; //SLOW CLOCK , WAVEFORM , DISABLE CLK ON RC COMPARE
	tc->Channel_0.IDR = 0xFF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ ΔΙΑΚΟΠΩΝ
	tc->Channel_0.IER = 0x10; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΜΟΝΟ ΤΟΥ RC COMPARE


	tc->Channel_0.CCR = 0x05; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΤΟΥ Timer

	pioa->PER 		= 	0xC00; //ΓΡΑΜΜΗ 11 ΚΑΙ 12: ΓΕΝΙΚΟΥ ΣΚΟΠΟΥ
	pioa -> PUER 	= 	0xC00; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΣΤΗ ΓΡΑΜΜΗ 11 ΚΑΙ 12: PULL−UP
	pioa -> ODR 	= 	0xC00; //ΓΡΑΜΜΗ 11 ΚΑΙ 12: ΛΕΙΤΟΥΡΓΙΑ ΕΙΣΟΔΟΥ

	while( (tmp = getchar()) != 'e')
	{
		switch (state)
		{
			case 0:
				pioa->SODR = 0x3FF; //ΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ LED
				state = WhatDoesTheButtonSay(state);
				if(state != 0) pioa->CODR = 0x3FF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ LED
				break;
			case 1:
				pioa->SODR = 0x001; //ΕΝΕΡΓΟΠΟΙΗΣΗ TOΥ ΠΡΩΤΟΥ LED
				for(i = 0;i < 9; i++){
					pioa->CODR = 0x001<<i;
					pioa->SODR = 0x001<<(i+1);
				}
				flag = WhatDoesTheButtonSay(state);
				if( flag == 2 ){
					state = 2;
				} else {
					state = 3;
				}
				break;
			case 2:
				pioa->SODR = 0x200; //ΕΝΕΡΓΟΠΟΙΗΣΗ TOΥ ΤΕΛΕΥΤΑΙΟΥ LED
				for(i = 0;i < 9; i++){
					pioa->CODR = 0x200>>i;
					pioa->SODR = 0x200>>(i+1);
				}
				flag = WhatDoesTheButtonSay(state);
				if( flag == 1 ){
					state = 1;
				} else {
					state = 4;
				}
				break;
			case 3:
				pioa->CODR = 0x3FF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ LED
				player1_score++;
				if( player1_score == 3 ){
					printf("-------------------------\nResult --> Player 1 Wins!\n Insert (bit)Coin..\n-------------------------\n");
					player1_score = 0;
					player2_score = 0;
				}
				state = 0;
				break;
			case 4:
				pioa->CODR = 0x3FF; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΟΛΩΝ ΤΩΝ LED
				player2_score++;
				if( player2_score == 3 ){
					printf("-------------------------\nResult --> Player 2 Wins!\n Insert (bit)Coin..\n-------------------------\n");
					player1_score = 0;
					player2_score = 0;
				}
				state = 0;
				break;
		}


	}

	tc->Channel_0.CCR = 0x02; //ΑΠΕΝΕΡΓΟΠΟΙΗΣΗ ΤΟΥ Timer

	CLEANUP;

	return 0;
}

void FIQ_handler(void)
{

	
}

int WhatDoesTheButtonSay(int state)
{
	unsigned int data_in = 0;
	int flag = 0;

	data_in = pioa->PDSR; //ΑΝΑΓΝΩΣΗ ΤΙΜΩΝ ΕΙΣΟΔΟΥ
		
	if( (data_in & 0x800) && ( state == 0 || state == 2) ) {//ΔΙΑΚΟΠΤΗΣ Player 1 ΠΑΤΗΜΕΝΟΣ(1000 0000 0000(b))
		flag = 1;
	}else if ( (data_in & 0x400) && ( state == 0 || state == 1) ) { //ΔΙΑΚΟΠΤΗΣ Player 2 ΠΑΤΗΜΕΝΟΣ(0100 0000 0000(b))
		flag = 2;
	}
	return flag;
}
