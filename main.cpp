/**************************************************************************
 *                             Version 1.0                                *
 **************************************************************************/
#include "wheelchair.h"
Serial pc(USBTX, USBRX, 57600);         // Serial Monitor

/**************************************************************************
 *                      Encoder Pins & Variables                          *
 **************************************************************************/
 
QEI wheel (D10, D9, NC, 1200);          // Initializes right encoder
DigitalIn pt3(D10, PullUp);             // Pull up resistors to read analog signals into digital signals
DigitalIn pt4(D9, PullUp);

QEI wheelS (D7, D8, NC, 1200);          // Initializes Left encoder
DigitalIn pt1(D7, PullUp);              // Pull up resistors to read analog signals into digital signals
DigitalIn pt2(D8, PullUp);

int max_velocity;

/**************************************************************************
 *        Variables and Pins for Watchdog and Emergency Button            *
 **************************************************************************/
 
DigitalIn e_button(D6, PullDown);       // Change to PullUp if testing without Emergency Button Connected
PwmOut on(PE_6);                        // Turn Wheelchair On
PwmOut off(PE_5);                       // Turn Wheelchair Off

//Watchdog limit should be 0.1; Set to 1 for Testing Only
double watchdogLimit = 1000000;               // Set timeout limit for watchdog timer in seconds
int buttonCheck = 0;
int iteration = 1;

/**************************************************************************
 *                      Joystick Pins & Variables                         *
 **************************************************************************/
 
AnalogIn x(A0);                         // Initializes analog axis for the joystick
AnalogIn y(A1);

DigitalOut up(D12);                     // Turn up speed mode for joystick 
DigitalOut down(D13);                   // Turn down speed mode for joystick 
bool manual = false;                    // Turns chair joystic to automatic and viceverza

/**************************************************************************
 *                      ToF Sensor Pin Assignments                        *
 **************************************************************************/

VL53L1X sensor1(PD_13, PD_12, PA_15);   // Block 1
VL53L1X sensor2(PD_13, PD_12, PC_7);
VL53L1X sensor3(PD_13, PD_12, PB_5);

VL53L1X sensor4(PD_13, PD_12, PE_11);   // Block 2
VL53L1X sensor5(PD_13, PD_12, PF_14);
VL53L1X sensor6(PD_13, PD_12, PE_13);

VL53L1X sensor7(PD_13, PD_12, PG_15);   // Block 3
VL53L1X sensor8(PD_13, PD_12, PG_14);
VL53L1X sensor9(PD_13, PD_12, PG_9);

VL53L1X sensor10(PD_13, PD_12, PE_10);  // Block 4
VL53L1X sensor11(PD_13, PD_12, PE_12);     
VL53L1X sensor12(PD_13, PD_12, PE_14);

VL53L1X* ToF[12] = {&sensor1, &sensor2, &sensor3, &sensor4, &sensor5, &sensor6, 
&sensor7, &sensor8, &sensor9, &sensor10, &sensor11, &sensor12}; // Puts ToF sensor pointers into an array
VL53L1X** ToFT = ToF;

/***h***********************************************************************
 *                          Thread Definitions                            *
 **************************************************************************/

Timer t, IMU_t;                                                 // Initialize time object t and IMU timer
EventQueue queue;                                               // Class to organize threads
Wheelchair smart(xDir,yDir, &pc, &IMU_t, &wheel, &wheelS, ToFT);    // Initialize wheelchair object
Thread compass;                                                 // Thread for compass
Thread velocity;                                                // Thread for velocity
Thread ToFSafe;                                                 // Thread for safety stuff
Thread emergencyButton;                                         // Thread to check button state and reset device

/**************************************************************************
 *                              MAIN CODE                                 *
 **************************************************************************/
int main(void)
{  
 
/*  nh.initNode();
    nh.advertise(chatter);
    nh.advertise(chatter2);
    nh.subscribe(sub); */
    
	pc.printf("Before Starting\r\n");
    
    //queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::compass_thread);        // Sets up sampling frequency of the compass thread
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::velocity_thread);         // Sets up sampling frequency of the velocity thread
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::ToFSafe_thread);          // Sets up sampling frequency of the ToF safety thread
    //queue.call_every(200, rosCom_thread);                                     // Sets up sampling frequency of the ROS com thread
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::emergencyButton_thread);  // Sets up sampling frequency of the emergency button thread
    
    t.reset();
    //compass.start(callback(&queue, &EventQueue::dispatch_forever));           // Starts running the compass thread
    velocity.start(callback(&queue, &EventQueue::dispatch_forever));            // Starts running the velocity thread
    ToFSafe.start(callback(&queue, &EventQueue::dispatch_forever));             // Starts running the ROS com thread
    //ros_com.start(callback(&queue, &EventQueue::dispatch_forever));           // Starts running the ROS com thread
    emergencyButton.start(callback(&queue, &EventQueue::dispatch_forever));     // Starts running the emergency button thread
    
    pc.printf("After Starting\r\n");

    Watchdog dog;                                                               // Creates Watchdog object
    dog.Configure(watchdogLimit);                                               // Configures timeout for Watchdog
    pc.printf("Code initiated\n");
    int set = 0;
    
    while(1) {
        if( pc.readable()) {
            set = 1;
            char c = pc.getc();                                                 // Read the instruction sent
            if( c == 'e') {
                smart.forward();                                                // Move foward
            }
            else if( c == 'a') {
                smart.left();                                                   // Turn left
            }
            else if( c == 'd') {
                smart.right();                                                  // Turn right
            }
            else if( c == 's') {
                smart.backward();                                               // Turn backwards
            }
            
            else if( c == 't') {                                        
                smart.pid_twistA();
            } 
            
            else if(c == 'v'){
                smart.showOdom();
            } 
            
            else if(c == 'o') {                                                 // Turns on chair
                pc.printf("turning on\r\n");
                on = 1;
                wait(1);
                on = 0;
            } 
            
            else if(c == 'f') {                                                 // Turns off chair
                pc.printf("turning off\r\n");
                off = 1;
                wait(1);
                off = 0;          
            } 
            
            else if(c == 'k'){                                                  // Sends command to go to the kitchen
                smart.pid_twistV();
            } 
            
            else if( c == 'm' || manual) {                                      // Turns wheelchair to joystick
                pc.printf("turning on joystick\r\n");
                manual = true;
                t.reset();
                while(manual) {
                    smart.move(x,y);                                            // Reads from joystick and moves
                    if( pc.readable()) {
                        char d = pc.getc();
                        if( d == 'm') {                                         // Turns wheelchair from joystick into auto
                            pc.printf("turning off joystick\r\n");
                            manual = false;
                        }
                    }
                }   
            }
            
            else {
                    pc.printf("none \r\n");
                    smart.stop();                                               // If nothing else is happening stop the chair
            }
        }
        
        else {    
            smart.stop();                                                       // If nothing else is happening stop the chair
        }
        
        wait(process);
        
        t.stop();
        //pc.printf("Time elapsed: %f seconds, Iteration = %d\n", t.read(), iteration);
        dog.Service();                                                          // Service the Watchdog so it does not cause a system reset - "Kicking"/"Feeding" the

    }
}

