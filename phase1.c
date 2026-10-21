#include <stdint.h>
#include <stdbool.h>
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

// -----------------------------------------
//  PHASE 1 - Only LCD
//  If this works, you will see on LCD:
//  Row 0: "  STOPWATCH   "
//  Row 1: "   READY...   "
//  Nothing else -- no buttons, no timer
// -----------------------------------------

void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t ch);
void LCD_Print(const char *text);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Init(void);

static void wait_ms(uint32_t ms)
{
    SysCtlDelay((SysCtlClockGet() / 3000) * ms);
}

static void wait_us(uint32_t us)
{
    uint32_t count = (SysCtlClockGet() / 3000000) * us;
    if(count < 5) count = 5;
    SysCtlDelay(count);
}

static void LCD_Nibble(uint8_t nibble, uint8_t rs)
{
    if(rs)
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2); // RS = 1
    else
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0);          // RS = 0

    GPIOPinWrite(GPIO_PORTD_BASE,
                 GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                 nibble & 0x0F);

    wait_us(1);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_PIN_3); // E HIGH
    wait_us(5);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0);           // E LOW
    wait_us(200);
}

void LCD_Command(uint8_t cmd)
{
    LCD_Nibble(cmd >> 4,   0);
    LCD_Nibble(cmd & 0x0F, 0);
}

void LCD_Data(uint8_t ch)
{
    LCD_Nibble(ch >> 4,   1);
    LCD_Nibble(ch & 0x0F, 1);
}

void LCD_Print(const char *text)
{
    while(*text)
        LCD_Data((uint8_t)(*text++));
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address = (row == 0) ? 0x00 : 0x40;
    LCD_Command(0x80 | (address + col));
}

void LCD_Init(void)
{
    wait_ms(100);

    LCD_Nibble(0x03, 0);  wait_ms(10);
    LCD_Nibble(0x03, 0);  wait_ms(5);
    LCD_Nibble(0x03, 0);  wait_ms(2);
    LCD_Nibble(0x02, 0);  wait_ms(2);

    LCD_Command(0x28);  wait_ms(2);  // 4-bit, 2 lines
    LCD_Command(0x08);  wait_ms(2);  // display OFF
    LCD_Command(0x01);  wait_ms(5);  // clear
    LCD_Command(0x06);  wait_ms(2);  // move cursor right
    LCD_Command(0x0C);  wait_ms(2);  // display ON
}

int main(void)
{
    // set clock to 16 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // turn on Port D and Port E clocks
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    // PD0-PD3 = LCD data lines (output)
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,
                          GPIO_PIN_0 | GPIO_PIN_1 |
                          GPIO_PIN_2 | GPIO_PIN_3);

    // PE2 = RS,  PE3 = E  (both output)
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // start LCD
    LCD_Init();

    // show static text
    LCD_SetCursor(0, 0);
    LCD_Print("  STOPWATCH   ");
    LCD_SetCursor(1, 0);
    LCD_Print("   READY...   ");

    // do nothing forever -- just keep the text on screen
    while(1)
    {
    }
}
