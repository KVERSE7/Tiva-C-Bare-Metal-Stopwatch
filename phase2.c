#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

// -----------------------------------------
//  PHASE 2 - LCD + Timer
//  Timer starts automatically on power on
//  Row 0 shows: "Time: 00:00.00"
//  Row 1 shows: "  STOPWATCH   "
//  No buttons yet -- timer just runs
// -----------------------------------------

volatile uint32_t g_ticks    = 0;  // counts 10ms ticks
volatile uint8_t  g_showTime = 0;  // flag to update LCD

void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t ch);
void LCD_Print(const char *text);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Init(void);
void FormatTime(uint32_t ticks, char *buf);
void SysTickHandler(void);

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
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2);
    else
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0);

    GPIOPinWrite(GPIO_PORTD_BASE,
                 GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                 nibble & 0x0F);

    wait_us(1);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_PIN_3);
    wait_us(5);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0);
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
    LCD_Command(0x28);  wait_ms(2);
    LCD_Command(0x08);  wait_ms(2);
    LCD_Command(0x01);  wait_ms(5);
    LCD_Command(0x06);  wait_ms(2);
    LCD_Command(0x0C);  wait_ms(2);
}

// converts tick count to "MM:SS.cc"
void FormatTime(uint32_t ticks, char *buf)
{
    uint32_t minutes = ticks / 6000;
    uint32_t seconds = (ticks / 100) % 60;
    uint32_t centi   = ticks % 100;
    snprintf(buf, 9, "%02lu:%02lu.%02lu", minutes, seconds, centi);
}

// called every 10 ms by hardware
void SysTickHandler(void)
{
    g_ticks++;       // count up
    g_showTime = 1;  // tell main loop to update LCD
}

int main(void)
{
    char timeBuf[9];

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,
                          GPIO_PIN_0 | GPIO_PIN_1 |
                          GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // SysTick: fires every 10 ms
    SysTickPeriodSet(SysCtlClockGet() / 100);
    SysTickIntRegister(SysTickHandler);
    SysTickIntEnable();
    SysTickEnable();

    LCD_Init();

    // show fixed label on row 1
    LCD_SetCursor(1, 0);
    LCD_Print("  STOPWATCH   ");

    while(1)
    {
        // update LCD every 10 ms when flag is set
        if(g_showTime)
        {
            g_showTime = 0;

            FormatTime(g_ticks, timeBuf);

            LCD_SetCursor(0, 0);
            LCD_Print("Time: ");
            LCD_Print(timeBuf);
        }
    }
}
