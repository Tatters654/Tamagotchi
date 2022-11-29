/*
 TAMAGOTCHI FUNCTIONS:
    -Press button 1 on the Sensortag to Feed Tamagotchi (Increases EAT value by 1)
    -Press the other button on Sensortag to Pet the Tamagotchi (Increases PET value by 1)
    -Any kind of continous movement with enough force (so that the Accelerometer values increase by at least a certain value)
     will count as exercise (Increases EXERCISE value by 1)

     **modify EXERCISE_THRESHOLD value on line 81 to adjust the movement required for Exercise value to increase -----------(MAY BE REQUIRED WHEN CODE IS LOADED TO A NEW SENSORTAG)----------

   When Sensortag receives a message from the backend, it flashes a led and makes a sound depending on the message the backend sent:
       Tamagotchi is well fed and doesn't want to be fed anymore: *Beeps once and green LED flashes*
       Tamagotchi wants to be fed: *Beeps once and red LED flashes*

       Tamagotchi doesn't want the owner to pet it: *Beeps twice and green LED flashes*
       Tamagotchi wants the owner to pet it: *Beeps twice and red LED flashes*

       Tamagotchi doesn't want to exercise: *Beeps thrice and green LED flashes*
       Tamagotchi wants to exercise: *Beeps thrice and red LED flashes*


       If Tamagotchi runs away, the Sensortag plays a short farewell song and flashes the red LED




Creators: Mikko Lempinen, Jere Tapsa & Nanna Setämaa
    Everyone took part in project planning, Jere was in charge of data handling, Nanna helped with whatever was needed and Mikko made sure the project got done

*/
/* C Standard library */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "buzzer.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"

/* Task */
#define STACKSIZE 2048
Char taskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char mpuTaskStack[STACKSIZE];

// Definition of the state machine
enum state
{
    WAITING = 1,
    MSG_READY,
    RECV_MSG
};
enum state programState = WAITING;

// Global variables
short uartPet = 0;
short uartEat = 0;
short uartExercise = 0;
char uartInput[400];
char uartBuffer[400];
//Make sure TAG_ID matches the ID on web application
char TAG_ID[] = "1111";
//The movement threshold required to increase Exercise value
double EXERCISE_THRESHOLD = 25.00;
//Variables for buzzer notes
#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_A3 220
#define NOTE_G3 196
#define NOTE_B3 247
#define NOTE_D4 294
// Add pins RTOS-variables and configuration here
static PIN_Handle buttonHandle0;
static PIN_State buttonState0;
static PIN_Handle buttonHandle1;
static PIN_State buttonState1;
static PIN_Handle ledHandle1;
static PIN_State ledState1;
static PIN_Handle ledHandle0;
static PIN_State ledState0;
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Handle hBuzzer;
static PIN_State sBuzzer;

PIN_Config cBuzzer[] = {
    Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};
PIN_Config buttonConfig0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE};
PIN_Config buttonConfig1[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE};

PIN_Config ledConfig0[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};
PIN_Config ledConfig1[] = {
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};
PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};

// I2C interface for MPU
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1};

void buttonFxn0(PIN_Handle handle, PIN_Id pinId)
{
    // Increase the uartEat value by 3 (i.e. Feed the pet)
    uartEat += 1;
    programState = MSG_READY;
}
void buttonFxn1(PIN_Handle handle, PIN_Id pinId)
{
    // Increase the uartPet value by 3 (i.e. Pet the pet)
    uartPet += 1;
    programState = MSG_READY;
}
void ledBlink(int color)
{
    // Blinks LEDs. Give the function value 0 or 1 depending on which led you want to blink
    //  0 = Green and 1 = RED
    if (color == 0)
    {
        PIN_setOutputValue(ledHandle0, Board_LED0, 1);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle0, Board_LED0, 0);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle0, Board_LED0, 1);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle0, Board_LED0, 0);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle0, Board_LED0, 1);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle0, Board_LED0, 0);
    }
    if (color == 1)
    {
        PIN_setOutputValue(ledHandle1, Board_LED1, 1);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle1, Board_LED1, 0);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle1, Board_LED1, 1);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle1, Board_LED1, 0);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle1, Board_LED1, 1);
        Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle1, Board_LED1, 0);
    }
}
void buzzerBuzz()
{
    // Make the buzzer go buzz!
    buzzerOpen(hBuzzer);
    buzzerSetFrequency(2000);
    Task_sleep(500000 / Clock_tickPeriod);
    buzzerClose();
}
void buzzerSong1()
{
    // Play a song using the buzzer
    buzzerOpen(hBuzzer);
    buzzerSetFrequency(NOTE_C4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_E4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_G4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_E4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_C4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_E4);
    Task_sleep(375000/2 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_G4);
    Task_sleep(562500 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_E4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_A3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_C4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_E4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_C4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_A3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_C4);
    Task_sleep(375000/2 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_E4);
    Task_sleep(562500 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_C4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_G3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_B3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_D4);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_B3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_G3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_B3);
    Task_sleep(375000/2 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_D4);
    Task_sleep(562500 / Clock_tickPeriod);
    buzzerSetFrequency(NOTE_B3);
    Task_sleep(375000 / Clock_tickPeriod);
    buzzerClose();
}

Void mpuFxn(UArg arg0, UArg arg1)
{
    double dataArray[8][6];
    float ax, ay, az, gx, gy, gz;
    double aX, aY, aZ, gX, gY, gZ;
    int counter = 0;
    double totalMoved = 0.00;
    double totalAx = 0.00;
    double totalAy = 0.00;
    double totalAz = 0.00;
    I2C_Handle i2cMPU;
    I2C_Params i2cMPUParams;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;
    // Power on MPU
    PIN_setOutputValue(hMpuPin, Board_MPU_POWER, Board_MPU_POWER_ON);
    // Wait 100ms for MPU sensor to power up
    Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();
    // Open MPU I2C
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL)
    {
        System_abort("Error Initializing I2CMPU\n");
    }
    // Setup MPU-sensor
    mpu9250_setup(&i2cMPU);
    // Close MPU I2C
    //I2C_close(i2cMPU);
    while (1)
    {
        // Open MPU I2C
        if (programState == WAITING)
        {
            // Get MPU data
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
            aX = (double)ax;
            aY = (double)ay;
            aZ = (double)az;
            gX = (double)gx;
            gY = (double)gy;
            gZ = (double)gz;
            if(counter == 0)
            {
                dataArray[0][0] = aX;
                dataArray[0][1] = aY;
                dataArray[0][2] = aZ;
                dataArray[0][3] = gX;
                dataArray[0][4] = gY;
                dataArray[0][5] = gZ;
            }
            if(counter == 1)
            {
                dataArray[1][0] = aX;
                dataArray[1][1] = aY;
                dataArray[1][2] = aZ;
                dataArray[1][3] = gX;
                dataArray[1][4] = gY;
                dataArray[1][5] = gZ;
            }
            if(counter == 2)
            {
                dataArray[2][0] = aX;
                dataArray[2][1] = aY;
                dataArray[2][2] = aZ;
                dataArray[2][3] = gX;
                dataArray[2][4] = gY;
                dataArray[2][5] = gZ;
            }
            if(counter == 3)
            {
                dataArray[3][0] = aX;
                dataArray[3][1] = aY;
                dataArray[3][2] = aZ;
                dataArray[3][3] = gX;
                dataArray[3][4] = gY;
                dataArray[3][5] = gZ;
            }
            if(counter == 4)
            {
                dataArray[4][0] = aX;
                dataArray[4][1] = aY;
                dataArray[4][2] = aZ;
                dataArray[4][3] = gX;
                dataArray[4][4] = gY;
                dataArray[4][5] = gZ;
            }
            if(counter == 5)
            {
                dataArray[5][0] = aX;
                dataArray[5][1] = aY;
                dataArray[5][2] = aZ;
                dataArray[5][3] = gX;
                dataArray[5][4] = gY;
                dataArray[5][5] = gZ;
            }
            if(counter == 6)
            {
                dataArray[6][0] = aX;
                dataArray[6][1] = aY;
                dataArray[6][2] = aZ;
                dataArray[6][3] = gX;
                dataArray[6][4] = gY;
                dataArray[6][5] = gZ;
            }
            if(counter == 7)
            {
                dataArray[7][0] = aX;
                dataArray[7][1] = aY;
                dataArray[7][2] = aZ;
                dataArray[7][3] = gX;
                dataArray[7][4] = gY;
                dataArray[7][5] = gZ;
            }
            counter++;
            if (dataArray[7][5] != NULL)
            {
                totalAx = fabs(dataArray[0][0]) + fabs(dataArray[1][0]) + fabs(dataArray[2][0]) + fabs(dataArray[3][0]) + fabs(dataArray[4][0]) + fabs(dataArray[5][0]) + fabs(dataArray[6][0]) + fabs(dataArray[7][0]);
                totalAy = fabs(dataArray[0][1]) + fabs(dataArray[1][1]) + fabs(dataArray[2][1]) + fabs(dataArray[3][1]) + fabs(dataArray[4][1]) + fabs(dataArray[5][1]) + fabs(dataArray[6][1]) + fabs(dataArray[7][1]);
                totalAz = fabs(dataArray[0][2]) + fabs(dataArray[1][2]) + fabs(dataArray[2][2]) + fabs(dataArray[3][2]) + fabs(dataArray[4][2]) + fabs(dataArray[5][2]) + fabs(dataArray[6][2]) + fabs(dataArray[7][2]);
                totalMoved = totalAx + totalAy + totalAz;
                if(totalMoved > EXERCISE_THRESHOLD)
                {
                    uartExercise += 1;
                    programState = MSG_READY;
                }
            }
            //Observe the MPU Sensor values if needed:
            //printf("MPU DATA: ax = %.2f, ay = %.2f, az = %.2f, gx = %.2f, gy = %.2f, gz = %.2f  \n", ax, ay, az, gx, gy, gz);

            if (counter > 7)
            {
                counter = 0;
            }
            // Close MPU I2C
            //I2C_close(i2cMPU);
        }
        // Sleep 1000ms
        Task_sleep(1000000 / Clock_tickPeriod);
    }
}

static void uartFxn(UART_Handle uart, void *rxBuffer, size_t len)
{
    // Handler function for incoming UART messages
    strcat(uartBuffer, rxBuffer);
    programState = RECV_MSG;
    if (uartBuffer[0] != TAG_ID[0])
    {
        uartBuffer[0] = '\0';
        UART_readCancel(uart);
    }
    UART_read(uart, uartInput, 1);
}

/* Task Functions */
Void uartTaskFxn(UArg arg0, UArg arg1)
{
    UART_Handle uart;
    UART_Params uartParams;
    char uartMsg[30];

    char NO_EAT[] = "Calm down";
    char NO_EXERCISE[] = "Too fitness";
    char NO_PET[] = "Feels good";
    char NEED_EAT[] = "low on food";
    char NEED_PET[] = "use a scratch";
    char NEED_EXERCISE[] = "Severe";
    char PET_GONE[] = "Too late";

    // UART connection setup as 9600,8n1
   UART_Params_init(&uartParams);
   uartParams.writeDataMode = UART_DATA_TEXT;
   uartParams.readDataMode = UART_DATA_TEXT;
   uartParams.readEcho = UART_ECHO_OFF;
   uartParams.readMode  = UART_MODE_CALLBACK; // Keskeytyspohjainen vastaanotto
   uartParams.readCallback  = &uartFxn; // Käsittelijäfunktio
   uartParams.baudRate = 9600; // nopeus 9600baud
   uartParams.dataLength = UART_LEN_8; // 8
   uartParams.parityType = UART_PAR_NONE; // n
   uartParams.stopBits = UART_STOP_ONE; // 1

    // Open UART Connection
    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL)
    {
        System_abort("Error opening the UART");
    }
    UART_read(uart, uartInput, 1);
    while (1)
    {
        //UART Read/Write loop
        if (programState == RECV_MSG)
        {
            // Read the input data and beep+blink led accordingly
            UART_close(uart);
            if (strstr(uartBuffer, TAG_ID) != NULL)
            {
                if (strstr(uartBuffer, NO_EAT) != NULL)
                {
                    ledBlink(0);
                    buzzerBuzz();
                }
                if (strstr(uartBuffer, NO_EXERCISE) != NULL)
                {
                    ledBlink(0);
                    buzzerBuzz();
                    //10ms
                    Task_sleep(100000 / Clock_tickPeriod);
                    buzzerBuzz();
                    //10ms
                    Task_sleep(100000 / Clock_tickPeriod);
                    buzzerBuzz();
                }
                if (strstr(uartBuffer, NO_PET) != NULL)
                {
                    ledBlink(0);
                    buzzerBuzz();
                    //10ms
                    Task_sleep(100000 / Clock_tickPeriod);
                    buzzerBuzz();
                }
                if (strstr(uartBuffer, NEED_EAT) != NULL)
                {
                    ledBlink(1);
                    buzzerBuzz();
                }
                if (strstr(uartBuffer, NEED_EXERCISE) != NULL)
                {
                    ledBlink(1);
                    buzzerBuzz();
                    //10ms
                    Task_sleep(100000 / Clock_tickPeriod);
                    buzzerBuzz();
                    //10ms
                    Task_sleep(100000 / Clock_tickPeriod);
                    buzzerBuzz();
                }
                if (strstr(uartBuffer, NEED_PET) != NULL)
                {
                    ledBlink(1);
                    buzzerBuzz();
                    //10ms
                    Task_sleep(100000 / Clock_tickPeriod);
                    buzzerBuzz();
                }
                if (strstr(uartBuffer, PET_GONE) != NULL)
                {
                    buzzerSong1();
                    ledBlink(1);
                    ledBlink(1);
                    ledBlink(1);
                    ledBlink(1);
                    ledBlink(1);
                    ledBlink(1);

                }
            }
            uartBuffer[0] = '\0';
            programState = WAITING;
            uart = UART_open(Board_UART0, &uartParams);
            UART_read(uart, uartInput, 1);
        }
        if (programState == MSG_READY)
        {
            // Send sensor data string with UART
            uartMsg[0] = '\0';
            sprintf(uartMsg, "id:1111,EAT:%d,PET:%d,EXERCISE:%d\0", uartEat, uartPet, uartExercise);
            size_t uartLen = strlen(uartMsg) + 1;
            UART_write(uart, uartMsg, uartLen);
            uartEat = 0;
            uartPet = 0;
            uartExercise = 0;
            uartMsg[0] = '\0';
            programState = WAITING;



        }
        // Once per 100ms, you can modify this
        Task_sleep(1000000 / Clock_tickPeriod);
    }

}

int main(void)
{
    // Task variables
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;
    Task_Handle mpuTaskHandle;
    Task_Params mpuTaskParams;

    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();

    // Initialize i2c bus
    Board_initI2C();

    // Initialize UART
    Board_initUART();

    // Open the pins
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (!hMpuPin)
    {
        System_abort("Error initializing MPU pins\n");
    }
    buttonHandle0 = PIN_open(&buttonState0, buttonConfig0);
    if (!buttonHandle0)
    {
        System_abort("Error initializing Button0 pins\n");
    }
    buttonHandle1 = PIN_open(&buttonState1, buttonConfig1);
    if (!buttonHandle1)
    {
        System_abort("Error initializing Button1 pins\n");
    }
    ledHandle0 = PIN_open(&ledState0, ledConfig0);
    if (!ledHandle0)
    {
        System_abort("Error initializing LED0 pins\n");
    }
    ledHandle1 = PIN_open(&ledState1, ledConfig1);
    if (!ledHandle1)
    {
        System_abort("Error initializing LED1 pins\n");
    }

    hBuzzer = PIN_open(&sBuzzer, cBuzzer);
    if (hBuzzer == NULL)
    {
        System_abort("Error initializing Buzzer pins\n");
    }

    if (PIN_registerIntCb(buttonHandle0, &buttonFxn0) != 0)
    {
        System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(buttonHandle1, &buttonFxn1) != 0)
    {
        System_abort("Error registering button callback function");
    }
    // MPU Task
    Task_Params_init(&mpuTaskParams);
    mpuTaskParams.stackSize = STACKSIZE;
    mpuTaskParams.stack = &mpuTaskStack;
    mpuTaskParams.priority = 2;
    mpuTaskHandle = Task_create(mpuFxn, &mpuTaskParams, NULL);
    if (mpuTaskHandle == NULL)
    {
        System_abort("MPU Task create failed!");
    }

    // uartTask
    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("uartTask create failed!");
    }

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();
    return (0);
}
