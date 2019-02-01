#include <msp430.h>
#include "peripherals.h"
#include "notes.h"                                                                                   //Include header file that has definition for the notes
#include "songs.h"                                                                                   //Include header file that has definition for the Notes struct and the songs array

// ****************--- Function Prototypes ---****************\\

char boardbuttons();
void resetGlobals();
void runtimerA2();
void runtimerA2fast();
void stopA2();
void flushbuffer();
void cleardisp();
void DACInit();
void ADCInit();
void DACSetValue(unsigned int dac_code);
unsigned int scrollWheel();

// ****************--- Declare globals ---**************** \\

unsigned int voltageDC = 0;                                                                             //voltage code value of DC and square wave
unsigned int sawtoothwave = 0;                                                                          //voltage code value of sawtooth wave
unsigned int trianglewave = 0;                                                                          //voltage code value of triangle wave
unsigned int countdown = 0;                                                                             //countdown timer, used for voltage of down edge of triangle wave
unsigned int frequency;                                                                                 //max_cnt of the modifiable timer A@
unsigned int triangleupdown = 1;                                                                        //1 means triangle wave going up, 0 means triangle wave going down
unsigned int cnt = 0;
char button;                                                                                            //check button pressed
unsigned int errors = 0;                                                                                //check errors made
unsigned int cntdwnmode = 4;                                                                            //cntdwnmode == 4 -> 3, ... , cntdwnmode == 0 -> GO
unsigned long int timer = 0;                                                                            //timer for interrupt
int m = 1;                                                                                              //for polling loop
unsigned char currentKey = 0;                                                                           //for checking numpad

// syntax for ISR

#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void)
{
    timer++;                                                                                            //increase timer by 1 each interrupt
    sawtoothwave += 74;
    countdown--;
}

typedef enum {MENU, DC, SQUARE, SAWTOOTH, TRIANGLE} state_t;                                            //declare all cases of state machine
state_t state = MENU;                                                                                   //first state is menu

// ****************--Main--**************** \\

void main(void)

{
     _BIS_SR(GIE);                                                                                      // Global Interrupt enable
    WDTCTL = WDTPW | WDTHOLD;                                                                           // Stop watchdog timer

//****************--- Peripherals Configuration ---**************** \\

    configDisplay();                                                                                    // LCD
    configKeypad();                                                                                     // Numpad
    configButtons();                                                                                    // 4 board buttons
    DACInit();                                                                                          // DAC
    ADCInit();                                                                                          // ADC

//****************-- State Machine --****************\\

    while (1)                                                                                      // Forever loop
    {
        switch(state)
        {
        case MENU:                                                                                                    //Main menu state
            resetGlobals();
            cleardisp();
            stopA2();
            Graphics_drawStringCentered(&g_sContext, "Function", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);       //Draw the menu
            Graphics_drawStringCentered(&g_sContext, "Generator", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "S1: DC", AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "S2: Square", AUTO_STRING_LENGTH, 48, 55, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "S3: Sawtooth", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "S4: Triangle", AUTO_STRING_LENGTH, 48, 75, TRANSPARENT_TEXT);
            Graphics_Rectangle box = {.xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
            Graphics_drawRectangle(&g_sContext, &box);
            flushbuffer();
            for (m = 0; m < 10000; m++)                                                                               //Polling loop
            {
                m=0;
                currentKey = boardbuttons();
                if(currentKey != 0)                                                                                   //If user press * starts to choose song1
                    break;
            }
            if(currentKey == 1)                                                                                       //If user press S1 choose DC
            {
                state = DC;
                resetGlobals();
                cleardisp();
                break;
            }
            else if(currentKey == 2)                                                                             //If user press S1 choose square wave
            {
                state = SQUARE;
                resetGlobals();
                cleardisp();
                break;
            }
            else if(currentKey == 3)                                                                             //If user press S3 choose sawtooth wave
            {
                state = SAWTOOTH;
                resetGlobals();
                cleardisp();
                break;
            }
            else if(currentKey == 4)                                                                             //If user press S4 choose triangle wave
            {
                state = TRIANGLE;
                resetGlobals();
                cleardisp();
                break;
            }
            break;
        case DC:                                                                                                 //DC generator
            Graphics_drawStringCentered(&g_sContext, "DC", AUTO_STRING_LENGTH, 48, 50, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press any button", AUTO_STRING_LENGTH, 48, 70, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "to return", AUTO_STRING_LENGTH, 48, 80, OPAQUE_TEXT);
            flushbuffer();
            while(1)
            {
                voltageDC = scrollWheel();                                                                       //DC voltage code value == code value of Voltage DC
                DACSetValue(voltageDC);                                                                          //Generate DC wave according to VDC
                currentKey = boardbuttons();
                if (currentKey != 0)                                                                             //press any key to return to menu
                {
                    state = MENU;
                    break;
                }
            }
                break;
        case SQUARE:                                                                                             //square wave generator
            Graphics_drawStringCentered(&g_sContext, "Square", AUTO_STRING_LENGTH, 48, 50, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press any button", AUTO_STRING_LENGTH, 48, 70, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "to return", AUTO_STRING_LENGTH, 48, 80, OPAQUE_TEXT);
            flushbuffer();
            unsigned int dacCodes[] = {0, 1489};                                                                 //min of square wave and max of square wave
            runtimerA2();
            while(1)
            {
                voltageDC = scrollWheel();
                dacCodes[1] = voltageDC;                                                                         //scrollwhell controls amplitude of square wave
                DACSetValue(dacCodes[timer & 1]);                                                                //alternate between 1 and 0 for each period
                currentKey = boardbuttons();
                if (currentKey != 0)                                                                             //press any key to return to menu
                {
                    state = MENU;
                    break;
                }
            }
            break;
        case SAWTOOTH:
            Graphics_drawStringCentered(&g_sContext, "Sawtooth", AUTO_STRING_LENGTH, 48, 50, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press any button", AUTO_STRING_LENGTH, 48, 70, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "to return", AUTO_STRING_LENGTH, 48, 80, OPAQUE_TEXT);
            flushbuffer();
            runtimerA2fast();                                                                                    //0.0005s/interrupt; 0.0005 * 20 = 0.01s = 100Hz
            while (1)
            {
                sawtoothwave = timer * 74;                                                                       //20 steps, each step 74/4095 * 3.3V = 0.0594V, pretty smooth wave, not too small
                if(sawtoothwave >= 1480)
                {
                    sawtoothwave = 0;                                                                            //when voltage reaches 1480 (20 steps), reset back to 0
                    timer = 0;
                }
                DACSetValue(sawtoothwave);
                currentKey = boardbuttons();
                if (currentKey != 0)                                                                             //press any key to return to menu
                {
                    state = MENU;
                    break;
                }
            }
            break;
        case TRIANGLE:
            Graphics_drawStringCentered(&g_sContext, "Triangle", AUTO_STRING_LENGTH, 48, 50, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press any button", AUTO_STRING_LENGTH, 48, 70, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "to return", AUTO_STRING_LENGTH, 48, 80, OPAQUE_TEXT);
            flushbuffer();
            while(1)
            {
                frequency = scrollWheel() + 20;                                                                //max_cnt of timerA2 controlled by scroll whell.
                runtimerA2freq(frequency);
                while (triangleupdown == 1)                                                                     //up edge of triangle wave
                {
                    trianglewave = timer * 74;                                                                  //20 steps, each step 74/4095 * 3.3V = 0.0594V
                    DACSetValue(trianglewave);
                    if (trianglewave >= 1480)                                                                   //after 20 steps, initiate down edge of triangle wave
                    {
                        triangleupdown = 0;                                                                     //change to down edge
                        timer = 0;                                                                              //restart timer
                        countdown = 20;                                                                         //20 steps, counting down
                        break;
                    }
                    currentKey = boardbuttons();                                                                //press any key to return to menu
                    if (currentKey != 0)
                        break;
                }
                while (triangleupdown == 0)                                                                     //down edge of triangle wave
                {
                    trianglewave = countdown * 74;                                                              //20 steps, each step 74/4095 * 3.3V = 0.0594V
                    if (trianglewave == 0 | trianglewave >= 65000)                                                           //if triangle wave <= 0 (unsigned int so triangle wave will roll around)
                    {
                        triangleupdown = 1;                                                                     //initiate up edge of triangle wave
                        timer = 0;
                        trianglewave = 0;
                    }
                    DACSetValue(trianglewave);
                    currentKey = boardbuttons();                                                                //press any key to return to menu
                    if (currentKey != 0)
                        break;
                }
                if (currentKey != 0)
                {
                    state = MENU;
                    break;
                }
            }
            break;
        }
    }// end while (1)
}

//****************-- Functions --****************\\

char boardbuttons()                                                                                   //Get board buttons, return a char according to the board button pressed
{
    char pressed = 0;
    if ((P7IN & BIT0)== 0x00)
        pressed = 1;
    if ((P3IN & BIT6)== 0x00)
        pressed = 2;
    if ((P2IN & BIT2)== 0x00)
        pressed = 3;
    if ((P7IN & BIT4)== 0x00)
        pressed = 4;
    return pressed;
}

void resetGlobals()                                                                                   //Reset global variables
{
    currentKey = '.';
    voltageDC = 0;
    timer = 0;
    voltageDC = 0;
    sawtoothwave = 0;
    trianglewave = 0;
    countdown = 0;
}
void runtimerA2(void)                                                                                 //Run Timer A2
{
    TA2CTL = (TASSEL_1 | MC_1 | ID_0);
    TA2CCR0 = 163;                                                                                    // 164 / 32768 = ~0.005sec resolution
    TA2CCTL0 = CCIE;
}

void runtimerA2fast()
{
    TA2CTL = (TASSEL_1 | MC_1 | ID_0);
    TA2CCR0 = 15;                                                                                     // 16 / 32768 = ~0.0005 sec resolution
    TA2CCTL0 = CCIE;
}
void runtimerA2freq(unsigned int freq)                                                                // run timer A2 with modifiable max_cnt
{
    TA2CTL = (TASSEL_2 | MC_1 | ID_0);
    TA2CCR0 = freq;
    TA2CCTL0 = CCIE;
}
void stopA2(void)                                                                                     //Stop Timer A2
{
    TA2CTL = MC_0;
    TA2CCTL0 &= ~CCIE;
}
void cleardisp()                                                                                      //Clear display, simplify syntax
{
    Graphics_clearDisplay(&g_sContext);
}

void flushbuffer()                                                                                    //Flush display, simplify syntax
{
    Graphics_flushBuffer(&g_sContext);
}

void DACInit()                                                                                        // Configure LDAC and CS for digital IO outputs
{
    DAC_PORT_LDAC_SEL &= ~DAC_PIN_LDAC;
    DAC_PORT_LDAC_DIR |=  DAC_PIN_LDAC;
    DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;                                                                // Deassert LDAC

    DAC_PORT_CS_SEL   &= ~DAC_PIN_CS;
    DAC_PORT_CS_DIR   |=  DAC_PIN_CS;
    DAC_PORT_CS_OUT   |=  DAC_PIN_CS;                                                                 // Deassert CS
}

void DACSetValue(unsigned int dac_code)
{

    DAC_PORT_CS_OUT &= ~DAC_PIN_CS;

    // Write in DAC configuration bits. From DAC data sheet
    // 3h=0011 to highest nibble.
    // 0=DACA, 0=buffered, 1=Gain=1, 1=Out Enbl
    dac_code |= 0x3000;     // Add control bits to DAC word

    uint8_t lo_byte = (unsigned char)(dac_code & 0x00FF);
    uint8_t hi_byte = (unsigned char)((dac_code & 0xFF00) >> 8);

    // First, send the high byte
    DAC_SPI_REG_TXBUF = hi_byte;

    // Wait for the SPI peripheral to finish transmitting
    while(!(DAC_SPI_REG_IFG & UCTXIFG)) {
        _no_operation();
    }

    // Then send the low byte
    DAC_SPI_REG_TXBUF = lo_byte;

    // Wait for the SPI peripheral to finish transmitting
    while(!(DAC_SPI_REG_IFG & UCTXIFG)) {
        _no_operation();
    }

    // We are done transmitting, so de-assert CS (set = 1)
    DAC_PORT_CS_OUT |=  DAC_PIN_CS;

    // This DAC is designed such that the code we send does not
    // take effect on the output until we toggle the LDAC pin.
    // This is because the DAC has multiple outputs. This design
    // enables a user to send voltage codes to each output and
    // have them all take effect at the same time.
    DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;  // Assert LDAC
     __delay_cycles(10);                 // small delay
    DAC_PORT_LDAC_OUT |=  DAC_PIN_LDAC;  // De-assert LDAC
}

void ADCInit()                                                                                    //initiate ADC
{
    REFCTL0 &= ~REFMSTR;
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON ;                                              // Set up ADC12, Multiple conversion,
    ADC12CTL1 = ADC12SHP;
    P6SEL |= BIT0;                                                                                // Port 6 pin 0 to function mode in ADC
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;                                                       // ref = 3.3V, ch = A0
    __delay_cycles(50);
    ADC12CTL0 |= ADC12ENC;                                                                        // Enable conversion
}

unsigned int scrollWheel()                                                                        //return the voltage of potentiomater
{
    unsigned int potReading = 0;
    ADC12CTL0 |= ADC12SC;
    while (ADC12CTL1 & ADC12BUSY)
            __no_operation();
    potReading = ADC12MEM0 & 0x0FFF; //keep low 12 bits
    return potReading;
}

