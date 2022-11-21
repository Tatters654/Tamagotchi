/* C Standard library */
#include <stdio.h>
#include <string.h>

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
char uartInput;
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
    uartEat += 3;
    programState = MSG_READY;
}
void buttonFxn1(PIN_Handle handle, PIN_Id pinId)
{
    // Increase the uartPet value by 3 (i.e. Pet the pet)
    uartPet += 3;
    programState = MSG_READY;
}
void ledBlink(int color)
{
    // Blinks LEDs. Give the function value 1 or 2 depending on which led you want to blink
    //  1 = Green and 2 = RED TODO:Check if this is correct or no
    if (color == 0)
    {
        PIN_setOutputValue(ledHandle0, Board_LED0, 1)
            Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle0, Board_LED0, 0)
    }
    if (color == 1)
    {
        PIN_setOutputValue(ledHandle1, Board_LED1, 1)
            Task_sleep(50000 / Clock_tickPeriod);
        PIN_setOutputValue(ledHandle1, Board_LED1, 0)
    }
}
void buzzerBuzz()
{
    // Make the buzzer go buzz!
    buzzerOpen(hBuzzer);
    buzzerSetFrequency(2000);
    Task_sleep(50000 / Clock_tickPeriod);
    buzzerClose();
}
Void mpuFxn(UArg arg0, UArg arg1)
{
    float ax, ay, az, gx, gy, gz;
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
    I2C_close(i2cMPU);
    // TEST EXERCISE:
    int counter = 0;

    while (1)
    {
        // Open MPU I2C
        if (programState == WAITING)
        {
            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            if (i2cMPU == NULL)
            {
                System_abort("Error Initializing I2CMPU\n");
            }
            // Get MPU data and print it to debugger
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
            printf("MPU DATA: ax = %.2f, ay = %.2f, az = %.2f, gx = %.2f, gy = %.2f, gz = %.2f  \n", ax, ay, az, gx, gy, gz);
            // TEST EXERCISE:
            counter++;
            if (counter > 10)
            {
                uartExercise = 3;
                programState == MSG_READY;
                counter = 0;
            }
            // Close MPU I2C
            I2C_close(i2cMPU);
        }
        // Sleep 100ms
        Task_sleep(100000 / Clock_tickPeriod);
    }
}

static void uartFxn(UART_Handle uart, void *rxBuffer, size_t len)
{
    // TODO:
    // Handler function for incoming UART messages
    programState = RECV_MSG;
}

/* Task Functions */
Void uartTaskFxn(UArg arg0, UArg arg1)
{
    // Exercise 4
    UART_Handle uart;
    UART_Params uartParams;
    char uartMsg[30];
    char TAG_ID[] = "1111";
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
    uartParams.readMode = UART_MODE_CALLBACK;
    uartParams.readCallback = &uartFxn;
    uartParams.baudRate = 9600;            // nopeus 9600baud
    uartParams.dataLength = UART_LEN_8;    // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE;   // 1

    // Open UART Connection
    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL)
    {
        System_abort("Error opening the UART");
    }
    while (1)
    {
        // Print out sensor data as string to debug window if the state is correct
        // Remember to modify state
        /*TODO:
         * Open/Close uart connection accordingly
         */
        if (programState == MSG_READY)
        {
            // Send sensor data string with UART
            sprintf(uartMsg, "id:1111,EAT:%d,PET:%d,EXERCISE:%d\0", uartEat, uartPet, uartExercise);
            UART_write(uart, uartMsg, strlen(uartMsg));
            uartEat, uartPet, uartExercise = 0;
            programState = WAITING;
        }
        // TODO: Add condition to read uart
        if (programState == RECV_MSG)
        {
            // Read the input data and beep+blink led accordingly
            UART_read(uart, &uartInput, 1);
            if (strstr(uart, TAG_ID) != NULL)
            {
                if (strstr(uart, NO_EAT) != NULL)
                {
                    ledBlink(0);
                    buzzerBuzz();
                }
                if (strstr(uart, NO_EXERCISE) != NULL)
                {
                    ledBlink(0);
                    buzzerBuzz();
                    buzzerBuzz();
                    buzzerBuzz();
                }
                if (strstr(uart, NO_PET) != NULL)
                {
                    ledBlink(0);
                    buzzerBuzz();
                    buzzerBuzz();
                }
                if (strstr(uart, NEED_EAT) != NULL)
                {
                    ledBlink(1);
                    buzzerBuzz();
                    buzzerBuzz();
                    buzzerBuzz();
                }
                if (strstr(uart, NEED_EXERCISE) != NULL)
                {
                    ledBlink(1);
                    buzzerBuzz();
                    buzzerBuzz();
                    buzzerBuzz();
                }
                if (strstr(uart, NEED_PET) != NULL)
                {
                    ledBlink(1);
                    buzzerBuzz();
                    buzzerBuzz();
                    buzzerBuzz();
                }
                if (strstr(uart, PET_GONE) != NULL)
                {
                    for (int i = 0; i < 6; i++)
                    {
                        ledBlink(1);
                    }
                }
            }
            programState = WAITING;
        }
    }
    // Once per 100ms, you can modify this
    Task_sleep(100000 / Clock_tickPeriod);
}

Void sensorTaskFxn(UArg arg0, UArg arg1)
{

    I2C_Handle i2c;
    I2C_Params i2cParams;
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the i2c bus
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL)
    {
        System_abort("Error Initializing I2C\n");
    }

    // Setup the OPT3001 sensor for use
    // Before calling the setup function, insert 100ms delay with Task_sleep
    Task_sleep(100000);
    opt3001_setup(&i2c);

    while (1)
    {
        // Read sensor data and print it to the Debug window as string
        ambientLight = opt3001_get_data(&i2c); // Valoisuus tallennettu globaaliin muuttujaan
        printf("sensorTask: %.4f Lux\n", ambientLight);

        // Modify state
        programState = MSG_READY;
        // Once per second, you can modify this
        Task_sleep(1000000 / Clock_tickPeriod);
    }
}

int main(void)
{
    /*
     * TODO: Aseta interupt h�ndlerit niin, etteiv�t sekoita koodia
     * eli silloin kuin mahdollisuus menn� sekaisin - ovat pois p��lt�
     * ja viimeisell� mahdollisella hetkell� interrupt handler p��lle ja takaisin pois p��lt�
     * katso Lovelace 18. Keskeytykset
     */
    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
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
    // Remember to register the above interrupt handler for button (Check below)
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

    // Set buttonFxn as interrupt handler for button pin
    if (PIN_registerIntCb(buttonHandle0, &buttonFxn0) != 0)
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
    // Task
    /*
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("sensorTask create failed!");
    }
    */
    // uartTask
    /*
    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("uartTask create failed!");
    }
    */
    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
