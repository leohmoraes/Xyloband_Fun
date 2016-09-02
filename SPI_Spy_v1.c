/* 
 * File:                SPI_Spy_v1.c
 * 
 * Author:              Rodders
 *                      https://roddersblog.wordpress.com/
 *                      https://twitter.com/rbell_uk
 *
 * Created:             28th August 2016
 * 
 * Config Control       28/08/2016  v1      Initial build
 *                      30/08/2016  v1.1    Changed to use ISR for SPI byte capture
 *                      01/09/2016  v1.2    Modified to use ASM, increase response
 *
 * Purpose:             project SPI-Spy was created to support reading the data tx between the Atmel MCU and the Si43xx
 *                      IC in a Coldplay Xyloband (UK #AHFODTour).
 * 
 *                      ENSURE SPI is interfaced to Xyloband correctly e.g. if SPI_SPY is running at 5v and Xyloband is 3v
 *                      will need level translator to boost SPI signals from Xyloband up to 5v
 *
 * Target:              PIC16F1789  (see www.Microchip.com)
 */

#include <xc.h>
#include "config.h"
#include <stdbool.h>			// Boolean (true/false) definition
#include <string.h>
#include "LCD_Module_1.h"

#define	_XTAL_FREQ	8000000		// Variable used for __delay_ms and __delay_us delays (must still set actual system clock))

#define SwRewind PORTBbits.RB1
#define SwBck PORTBbits.RB2
#define SwFwd PORTBbits.RB3
#define SwExit PORTBbits.RB4
#define SwStartStop PORTEbits.RE0

void initialPic(void);
void flash(int intFlashes);

void SPI_Init(void);

const char* ConvertByteToString(char);
const char* ConvertIntToString(unsigned int);

void DebounceTimer(void);

void SPI_Grab(void);

void TestSwitches();

//global variables

char Temp;

char Period_Count;                  //this flag increments on each 40us tick
bool boolFinished;                  //this flag keeps track of the end of the PWM Period
const char* myText;
int SPI_ID;
int SPI_Data[512];
bool Overflow;
bool Finished;

// ********************************************************************
void interrupt isr(void){

    char SPIByte;
    
    GIE=0;
        
    //find which interrupt using the flags and jump to service that particular interrupt

    //Capture a SPI byte? Likely another is loading as this code executes
    SPIByte=SSPBUF; //force read of SSPBUF, eg data from the SLAVE, and throw away. This resets BF.
    PIR1,SSP1IF=0;

    SPI_Data[SPI_ID]=SPIByte;
    //SPI_Data[SPI_ID]=SPI_ID;  //test purpose only
    LATAbits,LATA1=1;
    LATAbits,LATA1=0;            
    SPI_ID++;
    if (SPI_ID==512) {
        Finished=true;
        Overflow=true;
        PIE1bits.SSP1IE=false;
    }
            
    //finish    
    GIE=1;
}

// ********************************************************************
//void interrupt low_isr(void){ //only if device has two priority levels???

//}

// ********************************************************************

void initialPIC(void)
{   // Initialise user ports and peripherals.
    //clock set up; 64MHz
    OSCCONbits.IRCF0=0;
    OSCCONbits.IRCF1=1;
    OSCCONbits.IRCF2=1;
    OSCCONbits.IRCF3=1;
    OSCCONbits.SCS0=false;
    OSCCONbits.SCS1=false;

//    OSCTUNE? - not needed
    OSCCONbits.SPLLEN = 0;   //x4 clock even if config file sets pllen=0

    // alternaltive pin setup eg which pins are SCK and SDI and SS
    APFCON1=0b01000111;
    APFCON2=0b00000100;

    // Set port directions for I/O pins: 0 = Output, 1 = Input
    //PortA
    ANSELA = 0b00000000;
    TRISA = 0b00110001;	
    LATA = 0x00;
    
    //PortB
    ANSELB = 0b00000000;
    TRISB = 0b11011111;			// Keep RB6 and RB7 clear as ICSP only   

    //PortC
    LATC=0x00;     
    ANSELC = 0x00;
    TRISC = 0b00011000;  

    OPTION_REGbits.nWPUEN=0;
    WPUC = 0b00010000;
    INLVLC=0x00; 
    ODCONC=0x00;

    //PortD
    ANSELD = 0b00000000;
    WPUD=0b00000000;            //disable all weak pull-ups
    ODCOND=0b00000000;          //pull-pull output
    TRISD = 0b00000000;	

    //PortE
    ANSELE = 0b00000000;
    TRISE=0b00000111;
    WPUE=0b00000000;
    
    PSMC1CON=0x00;
    PSMC1OEN=0x00; 

    PIE1bits.SSP1IE=true;

    //INTCON setup
    PEIE=1;
    GIE=1;
}

// ********************************************************************
void SPI_Init(void)
{
    SSP1CON1=0b00000101;
    SSP1STAT=0b01000000;

    PIE1bits.SSP1IE=true;

    SSP1CON1bits.SSPEN = 1;
    PIR1bits.SSP1IF= 0;             // Clear interrupt flag
}



// ********************************************************************
void flash(int intFlashes)
// as a test utility
{
    for (int j=0; j< intFlashes;j++)
    {
        // Turn all LEDs on and wait for a time delay
        LATAbits.LATA1=1;
        for ( int i = 0; i < 100; i++ ) __delay_ms( 1 ); //see http://www.microchip.com/forums/m673703-p2.aspx
        // On the PIC18F series, the __delay_ms macro is limited to a delay of 179,200
        // instruction cycles (just under 15ms @ 48MHz clock).

        // Turn all LEDs off and wait for a time delay
        LATAbits.LATA1=0;
        for ( int i = 0; i < 100; i++ ) __delay_ms( 9 );
    }
}


//To write a 16 character string to one of the two LCD lines
void LCD_write(const char *String, bool bLine){
    int iPointer;
    char cCounter;
    bool bAddress;
    if (bLine==0) {
        LCD_busy();
        LCD_clear();
    }
    else{
        LCD_DDRAM(0xC0);
    }
    
    for ( iPointer = 0; iPointer<16; iPointer++ ) {
        LCD_busy();
        LCD_Data(String[iPointer]); 
    }   
}


const char* ConvertByteToString(char Value){

    char NibbleL;
    char NibbleH;
    static char OutputString[16];
    
    OutputString[0]='0';
    OutputString[1]='x'; 
    
        NibbleL = Value & 0b00001111;
        if (NibbleL < 10) {
            OutputString[3]=NibbleL+48;
        }    else{
            OutputString[3]=NibbleL+55;
        }
        
        Value=Value>>4;
        NibbleH=Value & 0b00001111;

        if (NibbleH < 10) {
            OutputString[2]=NibbleH+48;
        }    
        else{
            OutputString[2]=NibbleH+55;
        }
        
        for ( int i = 4; i < 16; i++ ) OutputString[i]=' ';
        
        return OutputString;
}

const char* ConvertIntToString(unsigned int Value){

    char NibbleL;
    char NibbleH;
    char ByteLow;
    char ByteHigh;
    
    static char OutputString[16];
    
    OutputString[0]='0';
    OutputString[1]='x'; 
    
    //Least Significant Byte
    ByteLow=Value & 0xFF;
    NibbleL = ByteLow & 0b00001111;
    if (NibbleL < 10) {
        OutputString[5]=NibbleL+48;
    }    
    else{
        OutputString[5]=NibbleL+55;
    }

    ByteLow=ByteLow>>4;
    NibbleH=ByteLow & 0b00001111;

    if (NibbleH < 10) {
        OutputString[4]=NibbleH+48;
    }    
    else{
        OutputString[4]=NibbleH+55;
    }
    
    ByteHigh=*((char*)&Value+1);
    NibbleL = ByteHigh & 0b00001111;
    if (NibbleL < 10) {
        OutputString[3]=NibbleL+48;
    }    
    else{
        OutputString[3]=NibbleL+55;
    }

    ByteHigh=ByteHigh>>4;
    NibbleH=ByteHigh & 0b00001111;

    if (NibbleH < 10) {
        OutputString[2]=NibbleH+48;
    }    
    else{
        OutputString[2]=NibbleH+55;
    }  
    
    for ( int i = 6; i < 16; i++ ) OutputString[i]=' ';
        
    return OutputString;
}


void DebounceTimer(void){
    __delay_ms(20);
}

void SPI_Grab(void){
//Gather SPI data until either (1) the storage space is full or (2) the stop button is pressed

    char ReturnByte;
    
    Finished = false;
    myText="Grabbing SPI    ";
    LCD_write(myText,0);
    myText="STOP?           ";
    LCD_write(myText,1);
    
    ReturnByte=0;
    
    while(!Finished){
        if(SwStartStop==0){     //no need to debounce, any 'touch, stops the capture.
            Finished=true;
        } //if 
    } //while
    
    LCD_clear();
}



int main(int argc, char** argv) {
    
char RegValue;
bool CycleValues;
int SPI_old;
    
    initialPIC();

    LATCbits,LATC1=1;
    LATCbits,LATC2=1;
    
    SPI_Init();

    initialLCD();
    LCD_clear();
       
    myText="SPI Spy v1      ";
    LCD_write(myText,0);
    myText="by Rodders      ";
    LCD_write(myText,1);        
    flash(4);
    LCD_clear();

  
    //The main GUI loop
    while(1){

        myText="Click START to  ";
        LCD_write(myText,0);
        myText="gather SPI data.";
        LCD_write(myText,1); 
        //Wait for Start...
        while(SwStartStop==1){ 
        }
        DebounceTimer();
        while(SwStartStop==0){ 
        }
        DebounceTimer();            

        PIE1bits.SSP1IE=true;
        SPI_ID=0;
        Overflow=false;

        SPI_Grab();           //returns -1 if more than 1024 bytes captured or >0 if user pressed stop

        if (Overflow){            
            myText="Warning:Overflow";
            LCD_write(myText,0);
            myText="> 512 Bytes     ";
            LCD_write(myText,1);    
        }
        else{
            myText="SPI Bytes       ";
            LCD_write(myText,0);
            myText="collected       ";
            LCD_write(myText,1);
        }
        flash(2);
        LCD_clear();


        SPI_old=0;
        CycleValues=true;
        //Now has data so allow user to cycle through and display contents
        myText="View Data       ";
        LCD_write(myText,0);
        myText="                ";
        LCD_write(myText,1);             

        while (CycleValues){

            //Rewind Button - move to the first ID)
            if(SwRewind==0){
                DebounceTimer();
                if(SwRewind==0){
                    while(SwRewind){}
                    DebounceTimer();
                    SPI_ID=0;                        
                }
            }

            //Back Button - move back one value
            if(SwBck==0){
                DebounceTimer();
                if (SwBck==0){
                    while(SwBck==0){}
                    DebounceTimer();                        
                    if (SPI_ID>0){
                    SPI_ID--;
                    }
                    else SPI_ID=0;    
                }
            }    

            //Forward Button - move forward one value
            if(SwFwd==0){
                DebounceTimer();
                if (SwFwd==0){
                    while(SwFwd==0){}

                    DebounceTimer();
                    if (SPI_ID<511){
                        SPI_ID++;
                    }
                    else SPI_ID=511;                            
                }
            }    

            //Exit Button - quit viewing data, go back to capture start
            if (SwExit==0){ 
                DebounceTimer();
                if (SwExit==0){
                    CycleValues=false;         //drop out of the examine loop and back to the start capture?
                    while(SwExit==0){}           //wait for release
                }
            }   

            //Display the SPI Byte/Data from SPI_Data where SPI_ID is as set
            if (SPI_ID!=SPI_old){
            myText=ConvertIntToString(SPI_ID);
            LCD_write(myText,0);
            myText=ConvertByteToString(SPI_Data[SPI_ID]);
            LCD_write(myText,1);
            SPI_old=SPI_ID;
            }

        } //while CycleValues  

    } //GUI Infinite Loop

    return (0); //(EXIT_SUCCESS);
}

