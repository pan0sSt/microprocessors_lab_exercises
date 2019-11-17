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






void FIQ_handler(void);

PIO* pioa = NULL;
AIC* aic = NULL;
TC* tc = NULL;

unsigned int temp=0; 
unsigned int button1_state = BUT_IDLE;
unsigned int button2_state = BUT_IDLE;
unsigned int game_state = 0;
unsigned int ball_pos;
unsigned int start=0;
unsigned int p1=0,p2=0;
unsigned int speed=8;
unsigned int counter1=0,counter2=0;
int wait1=0,wait2=0;

char tmp;

int main( int argc, const char* argv[] )
{
          STARTUP; 
          int gen;
          tc->Channel_0.RC = 2048;//kathe 0.25 sec tha exoume interupt tou clock  
          tc->Channel_0.CMR = 2084; 
          tc->Channel_0.IDR = 0xFF; 
          tc->Channel_0.IER = 0x10;  //energopoihsi tou rc compare
          
          aic->FFER = (1<<PIOA_ID) | (1<<TC0_ID); //oi diakopes apo tc0 kai pioA einai FIQ
          aic->IECR = (1<<PIOA_ID) | (1<<TC0_ID); //prowthisi diakopwn ston epe3ergasti
          
          
         
          pioa->PUER = 0xC00;   //pullup stous akrodektes 10,11
          pioa->ODR = 0xC00;   //san eisodos oi 10,11
          pioa->CODR = 0x3FF;  //oi e3odoi sto LOW
          pioa->OER = 0x3FF;   //ta pin san e3odoi
          pioa->SODR=0x11;//arxikopoiisi twn fanariwn stin K0
          
          
          
          //EKATHARISI PRWTA TCO KAI META aic POU INE PIO GENIKO
          gen = pioa->ISR; //katharismos apo proigoumena interupts me diavasma
          pioa->PER = 0xFFF;   //eidikou skopou kataxwrites 
          gen = tc->Channel_0.SR;//ekatharisi apo proigoumenes diakopes 
          aic->ICCR = (1<<PIOA_ID)|(1<<TC0_ID); //ekatharisi apo tyxon diakopes
          pioa->IER = 0xC00;   //enable ta interrupts sta pin 10,11
          
          tc->Channel_0.CCR = 0x05;//enarksi rologiou gia metrisi
          while( (tmp = getchar()) != 'e')
          {
                 
                 
          }
          
          aic->IDCR = (1<<PIOA_ID) | (1<<TC0_ID); //diakopi twn aic interrupts
          tc->Channel_0.CCR = 0x02;       //apenergopoihisi tou counter
          CLEANUP;
          return 0;
}
          

void FIQ_handler(void)
{
unsigned int data_in = 0x0;
unsigned int fiq = 0;
unsigned int data_out=0x0;
fiq = aic->IPR;    //apo pou egine interrupt

if( fiq & (1<<PIOA_ID) )
{ 
   data_in = pioa->ISR; //katharismos apo proigoumena interupts
   aic->ICCR = (1<<PIOA_ID); //ekatharisi apo tyxon diakopes
   data_in = pioa->PDSR; //anagnwsi timwn eisodou
   
   if( !(data_in & 0x800) )
    { 
        if(button1_state == BUT_IDLE&&wait1==0)
        {
           button1_state = BUT_PRESSED;
           wait1=4;
        }
     }//telos if gia interupt apo to START 
   else if(  !(data_in & 0x400) )         
    { 
        if(button2_state == BUT_IDLE&&wait2==0)
        {
           button2_state = BUT_PRESSED;
           wait2=4;
           
        }
     }//telos if gia interupt apo to STOP
 
     
}//Telos if gia interupt apo PIOA



if( fiq & (1<<TC0_ID) )
{
    temp++;
    data_out = tc->Channel_0.SR; //katharismos apo proigoumena interupts
    aic->ICCR = (1<<TC0_ID);    //ekatharisi apo tyxon diakopes
        
    if (button1_state==BUT_PRESSED && game_state==0)
       {    
          game_state=1;
          temp=0;
          ball_pos=0;
          pioa->CODR=0x3ff;
          pioa->SODR=0x001;
          
          
        }
        
    else if (button2_state==BUT_PRESSED && game_state==0)
       {    
          game_state=2;
          temp=0;
          ball_pos=9;
          pioa->CODR=0x3ff;
          pioa->SODR=0x200;
          
          
        }
        
    if (temp==speed && game_state==1)
      {
        if (wait1>=1) --wait1;
        temp=0;
        ball_pos++;
        ++counter1;
        if (ball_pos<=9)
        {
        pioa->CODR=0x3ff;
        pioa->SODR=(1<<ball_pos);      
        }//if gia ball_pos<=9
        
      }
    else if (temp==speed && game_state==2)
      {
        if (wait2>=1) --wait2;
        temp=0;
        ball_pos--;
        ++counter1;
        if (ball_pos>=0)
        {
        pioa->CODR=0x3ff;
        pioa->SODR=(1<<ball_pos);      
        }//if gia ball_pos>=0     
        
      }
    if (counter1>=18 && speed>=4)
    {
    speed=speed/2;
    counter1=0;
    }
      
      
    if ( (game_state==1) && (ball_pos==9) && (button2_state==BUT_PRESSED) )
      {
        game_state=2;
      }  
      
    else if ( (game_state==2) && (ball_pos==0) && (button1_state==BUT_PRESSED) )
      {
       game_state=1;
      }
      
    if (ball_pos==10)
        {
             p2++;
             temp=0;
             ball_pos=0;
             if (p2<=2)
             {
             game_state=-1;
             pioa->CODR=0x3ff;
             if (p1==0&&p2==1)
             pioa->SODR=0x001;
             else if(p1==1 && p2==2)
              pioa->SODR=(1<<9)|0x03;
             else if (p1==2 && p2==2) 
              pioa->SODR=(3<<8)|0x03;
             else if (p1==0 && p2==2)
              pioa->SODR=0x003;
              else if(p1==1 && p2==1)
               pioa->SODR=(1<<9)|0x001;
             
             }
             else
             {
                 pioa->CODR=0x3ff;
                 pioa->SODR=0x3ff;
                 game_state=0;
                 p1=0;
                 p2=0;
                 counter1=0;
                 speed=8;
                 wait1=0;
                 wait2=0;
             } 
         }
    else if (ball_pos==-1)
    {
             p1++;
             temp=0;
             ball_pos=0;
             if (p1<=2)
             {
             game_state=-1;
             pioa->CODR=0x3ff;
              if (p2==0&&p1==1)
             pioa->SODR=(1<<9);
             else if(p1==2 && p2==0)
              pioa->SODR=(3<<8);
             else if (p1==2 && p2==1) 
              pioa->SODR=(3<<8)|0x01;
              else if (p1==2 && p2==2)
              pioa->SODR=(3<<8)|0x03;
              else if(p1==1 && p2==1)
               pioa->SODR=(1<<9)|0x001;
             
             }
              else
             {
                 pioa->CODR=0x3ff;
                 pioa->SODR=0x3ff;
                 game_state=0;
                 p1=0;
                 p2=0;
                 speed=8;
                 counter1=0;
                 wait1=0;
                 wait2=0;
             } 
    }
    
    if ( (game_state==-1) && (temp==4))
    {
        pioa->CODR=0x3ff;
        game_state=0;
        speed=8;
        counter1=0;
       wait1=0;
       wait2=0;
         
    }
                         

   
        button1_state=BUT_IDLE;
        
   
        button2_state=BUT_IDLE;
        tc->Channel_0.CCR = 0x05;//enarksi rologiou gia metrisi

}//telos TCO Interupt


         

}//telos FIQ Handler   
