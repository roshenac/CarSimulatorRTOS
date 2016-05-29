//*******************************************************************
//                           Lab3-car simulation
//             simulation of car operation using a Real-Time Operating System (RTOS)
//
// Description
//  A real-time operating system is created in order so simulate a number of car processes. 
//  These include: brake, acceleration, odometry, speed, average speed, side lights, engine state 
//  and indicators. 
//
// Method
//  A thread scheduallar is used in order to carry out multiple processes. The maximum number of 
//  additional threads we were able to use was 10. As a result, several processes with the same
//  repetition rate had to go under a single thread.  
// 
//  4 semaphores are used which allows controlled access between these processes.  
// 
// Version
//    Roshenac Mitchell  March 2016

#include "MCP23017.h"
#include "WattBob_TextLCD.h"
#include "mbed.h"
#include "Servo.h"
#include "rtos.h"

// pointer to 16-bit parallel I/O object
MCP23017 *par_port; 

// pointer to 2*16 chacater LCD object 
WattBob_TextLCD *lcd; 

Serial serial(USBTX,USBRX);

//Input and Output ports
AnalogIn  acceleratorPedal(p17);    //Accelerator pedal
AnalogIn  brakePedal(p16);          //Brake pedal

DigitalIn engineSwitch(p27);        //Engine on/off switch
DigitalIn sideLightSwitch(p28);     //Side light on/off switch
DigitalIn leftIndicatorSwitch(p29); //Left indicator switch
DigitalIn rightIndicatorSwitch(p30);//Right indicator switch
 
Servo servo(p21);                   // External servo
DigitalOut OverSpeedLED(p20);       // speed warning light
DigitalOut engineLight(LED1);       //Engine on/off light
DigitalOut sideLight(LED2);         //Side light  
DigitalOut leftIndicator(LED3);     //Left turn indicator 
DigitalOut rightIndicator(LED4);    //Right turn indicator

// counter variable for read and write messages
int write = 0; 
int read =0;

// mail queue that stores 
//  - average speed
//  - acelerometer value
//  - break value 
typedef struct {
  float    speedVal; 
  float    accelerometerVal; 
  float    breakVal; 
} mail_t;

Mail<mail_t, 100> mail_box;
   
//Semaphores to manage accessing the viriables 
Semaphore CAR_MAIL_SEM(1);          // controls the read and sent messages
Semaphore INPUT_SEM(1);             // controls the read and sent inputs
Semaphore Speed_SEM(1);             // controls the calculated speed
Semaphore AVR_SPEED_SEM(1);         // controls the calculated average speed

// Local filesystem under the name "local" 
// This is used for writing to the csv file
LocalFileSystem local("local");   

// speed variables
const float maxSpeed = 140; //
float currentSpeed; 
float averageSpeed; 

// counter used to store last 3 speed values
// this is used when calculating average speed
const int sampleNumber = 3;
float speeds[sampleNumber];
int counter = 0;

// calculated or read values from the inputs
float accelerationValue;
float brakeValue;
int engineState;
int leftLightState; 
int rightLightState;
float odometerValue = 0; 


// Get the car acceleration and break and calculate speed
// speed semaphores are used so these values are not altered
// any anything process while getting the values. 
// repetition rate 20Hz = 0.05 seconds
void carSimulation(void const *args){
    while (true) {
        // calculate current speed from these values
        Speed_SEM.wait(); 
        // both acceleration and break value range between 0 and 1
        // engine state is either 0 or 1
        float totalAcc = (accelerationValue - brakeValue) * 100;
        float time = 0.05;
        currentSpeed = (currentSpeed + float(totalAcc * time)) * engineState;
        
        if(currentSpeed < 0)
        {
            currentSpeed = 0;
        }
        if(currentSpeed > maxSpeed)
        {
            currentSpeed = maxSpeed; 
        }
        
        Speed_SEM.release();
        
        // saves the last 3 speeds
        speeds[counter] = currentSpeed;       
        counter++;
        if(counter > 2)
        {
            counter = 0;
        }
           
        Thread::wait(50);
    }
}


// Read brake and accelerator values from variable resistors
// input semaphore is so these values are not changed while reading them
// Repetition rate 10Hz  =  0.1 seconds
void readBreakAndAccel(void const *args){
    while (true) {
        INPUT_SEM.wait();
        accelerationValue = acceleratorPedal.read();
        brakeValue =  brakePedal.read();
        INPUT_SEM.release();
        Thread::wait(100);
    }
} 


// Read engine on/off switch and show current state on an LED.
// input semaphore is so this values is not changed while reading it
// Repetition rate 2 Hz = 0.5 seconds
void readEngine(void const *args){
    while (true) {
        INPUT_SEM.wait();
        engineState = engineSwitch.read();
        INPUT_SEM.release();
        // switch engine light on or off respectively
        engineLight = engineState;   
        Thread::wait(500); 
    }
}


// Filter speed with averaging filter
// The last 3 speeds are used to caculate an average
// Both the speed and the average speed semaphores are used
// Repetition rate 5 Hz = 0.2 seconds
void getAverageSpeed(void const *args) {
    while (true) {
        int sum = 0; 
        Speed_SEM.wait();
        AVR_SPEED_SEM.wait();
        
        // get the sum of the last 3 speeds
        for(int i =0; i< sampleNumber ; i++)
        {
            sum += speeds[i];
        }
        // get the average of the last 3 speeds
        averageSpeed = sum/sampleNumber;
        
        AVR_SPEED_SEM.release();
        Speed_SEM.release();
        Thread::wait(200);
    } 
}


// Flash an LED if speed goes over 70 mph
// average speed semaphore is used so this value is not changed
// Repetition rate 0.5 Hz = 2 seconds
void speedOver70(void const *args){
    while(true){
        AVR_SPEED_SEM.wait();
        if(averageSpeed > 70)
        {
            // ! used to flip the values each time which
            // creates flashing.
            OverSpeedLED = !OverSpeedLED; 
        }else
        {
            OverSpeedLED = 0;
        }
        AVR_SPEED_SEM.release();        
        Thread::wait(2000);
    }       
}


// Send speed, accelerometer and brake values to a 100 element MAIL queue
// car mail semaphore used to protect messages
// average speed and input semphore used to fix vales
// Repetition rate 0.2 Hz = 5 seconds
void sendToMail(void const *args){
    while(true){
        mail_t *mail = mail_box.alloc();
        CAR_MAIL_SEM.wait();

        AVR_SPEED_SEM.wait();
        mail->speedVal = averageSpeed; 
        AVR_SPEED_SEM.release();
        
        INPUT_SEM.wait();
        mail->accelerometerVal = accelerationValue;
        mail->breakVal = brakeValue;
        INPUT_SEM.release();
        
        write++;        
       
        mail_box.put(mail);
                
        CAR_MAIL_SEM.release();
                
        Thread::wait(5000);  
    }
}


// Dump contents of feature (6) MAIL queue to the serial connection to the PC. 
// (Data will be passed through the MBED USB connection)
// content is also dumped into a csv file 
// car mail semaphore used to protect messages
// Repetition rate 0.05 Hz = 20 seconds
void dumpContents(void const *args){
    while(true){
        CAR_MAIL_SEM.wait();
        while(write > read){
            osEvent evt = mail_box.get();
            if (evt.status == osEventMail) 
            { 
                mail_t *mail = (mail_t*)evt.value.p;           
                
                // values sent to csv file
                FILE *fp = fopen("/local/Car_Values.csv", "a"); 
                fprintf(fp,"%f ,", mail->speedVal);
                fprintf(fp,"%f ,", mail->accelerometerVal);
                fprintf(fp,"%f ", mail->breakVal);
                fprintf(fp,"\r\n");
                fclose(fp); 
                
                // values sent to serial port
                serial.printf("average speed: %f ,", mail->speedVal);
                serial.printf("break value: %f ,", mail->breakVal);
                serial.printf("acceleration: %f ,", mail->accelerometerVal);
                serial.printf("\r\n");
                mail_box.free(mail);
                read++;
            }
        }
        CAR_MAIL_SEM.release();
        Thread::wait(20000);  
    }
}


// Read the two turn indicator switches.
// input semaphore used to protect values 
// Repetition rate 0.5 Hz = 2 seconds
void getIndicators(void const *args){
    while(true){
        INPUT_SEM.wait();
        leftLightState = leftIndicatorSwitch;
        rightLightState = rightIndicatorSwitch;
        INPUT_SEM.release();
        Thread::wait(2000);
    }
}

// -------------- Repetition rate 1 Hz ---------

// Show the average speed value with a RC servo motor
// average speed semaphore is used to hold this value constant
// Repetition rate 1 Hz = 1 second
void showAverageSpeed(){
        AVR_SPEED_SEM.wait();
        // scales the average speed to the max allowed speed
        // servo value is between 0 and 1
        servo = 1.0 - (averageSpeed / maxSpeed) ; 
        AVR_SPEED_SEM.release();
}


// Read a single side light switch and set side lights accordingly
// input semaphore used to hold value
// Repetition rate 1 Hz = 1 second
void readSideLight(){
        INPUT_SEM.wait();
        int sideLightState = sideLightSwitch;
        INPUT_SEM.release();
        sideLight = sideLightState; 
}


// Flash appropriate indicator LEDs at a rate of 1Hz
// input semaphore used so value doesnt change
// Repetition rate 1 Hz = 1 seconds
void flashIndicator()
{
    INPUT_SEM.wait();
    // only happens if a single light or no light is on
    if(!(leftLightState && rightLightState))
    { 
        if(leftLightState)
        {
            // ! used to flip value to create flashing
            leftIndicator = !leftIndicator;
            rightIndicator = 0;
        }            
        if(rightLightState) 
        {
            leftIndicator = 0;
            // ! used to flip value to create flashing
            rightIndicator = !rightIndicator;
         }
     }
     INPUT_SEM.release();
}


// single thread used to call multiple processes that 
// have a 1 Hz repetition rate
void oneHertz(void const *args)
{
    while(true)
    {
        flashIndicator();
        readSideLight();
        showAverageSpeed();
        Thread::wait(1000);
    }    
}


// -------------- Repetition rate 2 Hz ---------

// If both switches are switched on then flash both indicator LEDs at a rate of 2Hz (hazard mode).
// input semaphore used so value doesnt change
// Repetition rate 2 Hz = 0.5 seconds
void flashHazard()
{
    INPUT_SEM.wait();
    if(leftLightState && rightLightState)
    {
        leftIndicator = !leftIndicator;
        rightIndicator = leftIndicator;
    }
    INPUT_SEM.release();
}


// Update the odometer value
// Shows values of LCD display
//  - odometer values
//  - average speed
// average speed semaphone used to fix this value
// Repetition rate 2 Hz = 0.5 seconds 
void updateOdometer(){
        AVR_SPEED_SEM.wait(); 
        float time = 0.5;
        odometerValue += averageSpeed / time ;
        
         //show on MBED text display
        lcd->locate(0,0);
        lcd->printf("odo : %.0f", odometerValue);

        // show average speed   
        lcd->locate(1,0);
        lcd->printf("speed : %.2f", averageSpeed);

        AVR_SPEED_SEM.release(); 
}


// single thread used to call multiple processes that 
// have a 2 Hz repetition rate
void twoHertz(void const *args)
{
    while(true)
    {
        flashHazard();
        updateOdometer();
        Thread::wait(500);
    }    
}



int main() {

     // initialise 16-bit I/O chip
    par_port = new MCP23017(p9, p10, 0x40); 
    
    serial.baud(115200);
    
    // set up for the LCD
    lcd = new WattBob_TextLCD(par_port); // initialise 2*26 char display
    lcd->cls(); 
    par_port->write_bit(1,BL_BIT); // turn LCD backlight ON 
  
    // CREATE CSV FILE TO WRITE VALUES TO 
    FILE *fp = fopen("/local/Car_Values.csv", "w");  
    fprintf(fp, "Average_Speed,Accelerometer_Value,Brake_Value\r\n");
    fclose(fp); 
    
    //Define the multy thread function
    Thread Car_Simulation_Thread(carSimulation);                            
    Thread Read_Brake_And_Accel_Thread(readBreakAndAccel);
    Thread Read_Engine_Thread(readEngine);
    Thread Get_Average_Speed_Thread(getAverageSpeed);
    Thread Is_Over_70_Thread(speedOver70);
    Thread Send_To_Mail_Thread(sendToMail);
    Thread Dump_Contents_Thread(dumpContents);
    Thread Get_Indicators_Thread(getIndicators);
    Thread One_Hertz_Thread(oneHertz);
    Thread Two_Hertz_Thread(twoHertz);
    
    while(true)
    {
    }
}