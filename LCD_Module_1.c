/* 
 * File:                LCD_Module_1.c
 * 
 * Author:              Rodders
 *                      https://roddersblog.wordpress.com/
 *                      https://twitter.com/rbell_uk
 *
 * Created:             10th August 2016
 * 
 * Config Control       10/08/2016  v1      Initial build
 *
 * Purpose:             main source code for driving 2x20 character LCD module
 * 
 *                      uses 8 bit control
 *
 * Target:              PIC16F1789  (see www.Microchip.com)
 */

#include <xc.h>
#include <stdbool.h>
#include "LCD_Module_1.h"
#define	_XTAL_FREQ	8000000		// Variable used for time delays only

void initialLCD(void)
{
    __delay_ms( 200 );
    __delay_ms( 200 );
    __delay_ms( 200 );
     
    LCD_Instruction(0x30);      // 1st RESET    
    __delay_ms( 5 ); 

    LCD_Instruction(0x30);      // 2nd RESET    
    __delay_us( 200 );
    
//    LCD_Instruction(0x30);      // 3rd RESET    
    LCD_busy();
  
    LCD_Instruction(0x38);      //first actual meaningful function     
    LCD_busy();
    
    LCD_Instruction(0x0F);    
    LCD_busy();
    
    LCD_Instruction(0x01);      //clear display    
    LCD_busy();
    
    
    LCD_Instruction(0x06);      // Entry Mode    
    LCD_busy();  
        
}

// All functions/commands
void LCD_Instruction(char Value){
    __delay_us(1);
    LATCbits,LATC0=0;   
    LATCbits,LATC1=0;
    __delay_us(1);
    LATCbits,LATC2=1;
    __delay_us(1);
    LATAbits,LATA2=0;               // for 74HCT573 & 3.3/5v operation
    LATD=Value;
    __delay_us(1);
    LATCbits,LATC2=0;
    __delay_us(1);    
     LATCbits,LATC0   = 1;           //We are reading RW
     LATCbits,LATC1   = 1;           //We are reading RW
     LATCbits,LATC2   = 0;           //Enable H->
     __delay_us(1);
}

// Character data to screen
void LCD_Data(char Value){
    __delay_us(1);
    LATCbits,LATC0=1;  
    LATCbits,LATC1=0;
    __delay_us(1);    
    LATCbits,LATC2=1;
    __delay_us(1);
    LATAbits,LATA2=0;           // for 74HCT573 & 3.3/5v operation
    LATD=Value;
    __delay_us(1);
    LATCbits,LATC2=0;
    __delay_us(1);    
     LATCbits,LATC0   = 1;           //We are reading RW
     LATCbits,LATC1   = 1;           //We are reading RW
     LATCbits,LATC2   = 0;          //Enable H->
     __delay_us(1);
}

// Set position on screen
void LCD_DDRAM(char Value){
    __delay_us(1);
    LATCbits,LATC0=0;  
    LATCbits,LATC1=0;
    __delay_us(1);    
    LATCbits,LATC2=1;
    __delay_us(1);
    LATAbits,LATA2=0;           // for 74HCT573 & 3.3/5v operation
    LATD=Value;
    __delay_us(1);
    LATCbits,LATC2=0;
    __delay_us(1); 
     LATCbits,LATC0   = 1;           //We are reading RW
     LATCbits,LATC1   = 1;           //We are reading RW
     LATCbits,LATC2   = 0;      //Enable H->
     __delay_us(1);   
}

void LCD_busy()
{
    __delay_us(1);
    PORTD=0x00;

     TRISDbits,TRISD7   = 1;           //Make D7th bit of LCD as i/p
    __delay_us(1);
     LATCbits,LATC0   = 0;           //Selected command register
     LATCbits,LATC1   = 1;           //We are reading RW
     LATAbits,LATA2=1;           // for 74HCT573 & 3.3/5v operation; disable the outputs of latch
     
    __delay_us(1);
    LATCbits,LATC2=1;
    
     while(PORTDbits,RD7){          //read busy flag again and again till it becomes 0
     }
     LATCbits,LATC2   = 0;     //Enable H->
     __delay_us(1);     
     
     TRISD = 0x00;			// PWM RD0-RD7 = x8 pins    
     LATCbits,LATC0   = 1;           //We are reading RW
     LATCbits,LATC1   = 1;           //We are reading RW
     LATCbits,LATC2   = 0;     //Enable H->
     __delay_us(1);
     
     TRISD = 0x00;			// PWM RD0-RD7 = x8 pins
}

void LCD_clear(){

        LCD_busy(); 
        LCD_Instruction(0x01);
        __delay_ms(100);
}

void LCD_home(){
    
        LCD_busy(); 
        LCD_Instruction(0x02);
        __delay_ms(100); 
}
