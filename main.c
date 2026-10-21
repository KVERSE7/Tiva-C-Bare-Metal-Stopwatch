#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

// -----------------------------------------
//  PHASE 4 - COMPLETE STOPWATCH
//  SW1 = Start / Stop
//  SW2 = Record Lap
//  Green LED  = running
//  Red LED    = stopped
//  Blue LED   = flashes on lap
//  LCD Row 0  = current time
//  LCD Row 1  = last lap time
//  UART       = sends data to PC terminal
// -----------------------------------------

volatile uint32_t g_ticks    = 0;
volatile uint8_t  g_running  = 0;
volatile uint8_t  g_showTime = 0;
volatile uint32_t g_lapTick  = 0;
volatile uint8_t  g_showLap  = 0;

void UARTSend(const char *text);
void FormatTime(uint32_t ticks, char *buf);
void ShowLED(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t ch);
void LCD_Print(const char *text);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Init(void);
void SysTickHandler(void);
void ButtonHandler(void);

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

void FormatTime(uint32_t ticks, char *buf)
{
    uint32_t minutes = ticks / 6000;
    uint32_t seconds = (ticks / 100) % 60;
    uint32_t centi   = ticks % 100;
    snprintf(buf, 9, "%02lu:%02lu.%02lu", minutes, seconds, centi);
}

void ShowLED(void)
{
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);
    if(g_running)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3); // Green
    else
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1); // Red
}

void UARTSend(const char *text)
{
    while(*text)
    {
        UARTCharPut(UART0_BASE, *text);
        text++;
    }
}

void SysTickHandler(void)
{
    if(g_running)
    {
        g_ticks++;
        g_showTime = 1;
    }
}

void ButtonHandler(void)
{
    uint32_t pressed = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, pressed);

    // SW1 (PF4) -- Start or Stop
    if(pressed & GPIO_INT_PIN_4)
    {
        g_running ^= 1;
        ShowLED();

        if(g_running)
            UARTSend(">> RUNNING\r\n");
        else
            UARTSend(">> STOPPED\r\n");
    }

    // SW2 (PF0) -- Record Lap (only when running)
    if(pressed & GPIO_INT_PIN_0)
    {
        if(g_running)
        {
            g_lapTick = g_ticks;  // freeze lap time
            g_showLap = 1;        // tell main loop to show it

            // flash blue LED for 100 ms
            GPIOPinWrite(GPIO_PORTF_BASE,
                         GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                         GPIO_PIN_2);               // Blue ON
            SysCtlDelay(SysCtlClockGet() / 30);    // ~100 ms
            ShowLED();                              // back to Green
        }
    }
}

int main(void)
{
    char timeBuf[9];
    char lapBuf[9];
    char msg[40];

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // enable all clocks
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    // unlock PF0 (SW2 is a locked pin)
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR)  |= GPIO_PIN_0;

    // LCD pins
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,
                          GPIO_PIN_0 | GPIO_PIN_1 |
                          GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // LED pins
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
                          GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    // Button pins (input + pull-up)
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Button interrupts (falling edge)
    GPIOIntTypeSet(GPIO_PORTF_BASE,
                   GPIO_PIN_4 | GPIO_PIN_0, GPIO_FALLING_EDGE);
    GPIOIntRegister(GPIO_PORTF_BASE, ButtonHandler);
    GPIOIntEnable(GPIO_PORTF_BASE,
                  GPIO_INT_PIN_4 | GPIO_INT_PIN_0);

    // UART pins
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8  |
                         UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));

    // SysTick every 10 ms
    SysTickPeriodSet(SysCtlClockGet() / 100);
    SysTickIntRegister(SysTickHandler);
    SysTickIntEnable();
    SysTickEnable();

    IntMasterEnable();

    // start LCD and show welcome
    LCD_Init();
    LCD_SetCursor(0, 0);
    LCD_Print("Time: 00:00.00");
    LCD_SetCursor(1, 0);
    LCD_Print("Press SW1:Start");

    UARTSend("Stopwatch Ready\r\n");
    UARTSend("SW1=Start/Stop  SW2=Lap\r\n");

    ShowLED();  // Red LED (stopped)

    while(1)
    {
        // update time on LCD row 0
        if(g_showTime)
        {
            g_showTime = 0;
            FormatTime(g_ticks, timeBuf);
            LCD_SetCursor(0, 0);
            LCD_Print("Time: ");
            LCD_Print(timeBuf);
            snprintf(msg, sizeof(msg), "Time: %s\r\n", timeBuf);
            UARTSend(msg);
        }

        // show lap time on LCD row 1
        if(g_showLap)
        {
            g_showLap = 0;
            FormatTime(g_lapTick, lapBuf);
            LCD_SetCursor(1, 0);
            LCD_Print("Lap:  ");
            LCD_Print(lapBuf);
            snprintf(msg, sizeof(msg), "LAP:  %s\r\n", lapBuf);
            UARTSend(msg);
        }
    }
}
