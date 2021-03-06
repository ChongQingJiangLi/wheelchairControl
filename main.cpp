/**************************************************************************
 *                             Version 1.0                                *
 **************************************************************************/
#include "wheelchair.h"
Serial pc(USBTX, USBRX, 57600);         // Serial Monitor

/**************************************************************************
 *                      Encoder Pins & Variables                          *
 **************************************************************************/
 
QEI wheelS (PC_6, PC_8, NC, 1200);          	// Initializes right encoder
DigitalIn pt1(PC_8, PullUp);             	// Pull up resistors to read analog signals into digital signals
DigitalIn pt2(PC_6, PullUp);

QEI wheel (PD_2, PC_11, NC, 1200);         // Initializes Left encoder
DigitalIn pt3(PD_2, PullUp);        		// Pull up resistors to read analog signals into digital signals
DigitalIn pt4(PC_11, PullUp);

int max_velocity;

/**************************************************************************
 *        Variables and Pins for Watchdog and Emergency Button            *
 **************************************************************************/
 
DigitalIn e_button(PC_9, PullDown);      // Change to PullUp if testing without Emergency Button Connected
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

VL53L1X sensor1(PF_0, PF_1, PE_1);   // Block 1
VL53L1X sensor2(PF_0, PF_1, PG_9);
VL53L1X sensor3(PF_0, PF_1, PG_12);

VL53L1X sensor4(PF_0, PF_1, PG_10);   // Block 2
VL53L1X sensor5(PF_0, PF_1, PG_15);
VL53L1X sensor6(PF_0, PF_1, PA_15);

VL53L1X sensor7(PF_0, PF_1, PG_8);   // Block 3
VL53L1X sensor8(PF_0, PF_1, PG_5);
VL53L1X sensor9(PF_0, PF_1, PG_6);

VL53L1X sensor10(PF_0, PF_1, PB_12);  // Block 4
VL53L1X sensor11(PF_0, PF_1, PB_11);
VL53L1X sensor12(PF_0, PF_1, PB_2);

VL53L1X sensor13(PF_0, PF_1, PE_9);  // Block 5 //Right
VL53L1X sensor14(PF_0, PF_1, PB_1);				//Left

VL53L1X* ToF[14] = {&sensor1, &sensor2, &sensor3, &sensor4, &sensor5, &sensor6,
&sensor7, &sensor8, &sensor9, &sensor10, &sensor11, &sensor12, &sensor13, &sensor14}; // Puts ToF sensor pointers into an array

/**************************************************************************
 *                      ToF ARRAY ASSIGNMENTS
 *           (from the perspective of user seated on wheelchair)
 *
 *   Each ToF has a 3 letter name, the first indicates left/right, the
 *   second front/back, and third, the specific ToF sensor
 *   eg: LBB means Left side, Back end, Bottom ToF
 *
 *   FRONT - LEFT
 *   ToF 10	- Top (Angle)		LFT
 *   ToF 9	- Bottom (Front)	LFB
 *   ToF 11	- Side				LFS
 *
 *   FRONT - RIGHT
 *   ToF 8	- Top (Angle)		RFT
 *   ToF 7	- Bottom (Front)	RFB
 *   ToF 6	- Side				RFS
 *
 *   BACK - LEFT
 *   ToF 3	- Side				LBS
 *   ToF 4	- Top (Angle)		LBT
 *   ToF 5	- Bottom			LBB
 *
 *   BACK - RIGHT
 *   ToF 1	- Side				RBS
 *   ToF 2	- Top (Angle)		RBT
 *   ToF 0	- Bottom			RBB
 *
 **************************************************************************/

//
/*
VL53L1X sensor1(PF_1, PF_0, PG_12);   // Block 1
VL53L1X sensor2(PF_1, PF_0, PG_9);
VL53L1X sensor3(PF_1, PF_0, PE_1);

VL53L1X sensor4(PF_1, PF_0, PA_15);   // Block 2
VL53L1X sensor5(PF_1, PF_0, PA_14);
VL53L1X sensor6(PF_1, PF_0, PA_13);

VL53L1X sensor7(PF_1, PF_0, PG_8);   // Block 3
VL53L1X sensor8(PF_1, PF_0, PG_5);
VL53L1X sensor9(PF_1, PF_0, PG_6);

VL53L1X sensor10(PF_1, PF_0, PB_2);  // Block 4
VL53L1X sensor11(PF_1, PF_0, PB_1);
VL53L1X sensor12(PF_1, PF_0, PB_15);

VL53L1X sensor13(PF_1, PF_0, PF_04);  // Middle Block - Inward ToF sensors
VL53L1X sensor14(PF_1, PF_0, PE_9);

VL53L1X* ToF[14] = {&sensor1, &sensor2, &sensor3, &sensor4, &sensor5, &sensor6,
&sensor7, &sensor8, &sensor9, &sensor10, &sensor11, &sensor12, &sensor13, &sensor14}; // Puts ToF sensor pointers into an array
*/
//

VL53L1X** ToFT = ToF;

/**************************************************************************
 *                          Thread Definitions                            *
 **************************************************************************/
Timer t, IMU_t;                                                 // Initialize time object t and IMU timer
EventQueue queue;                                               // Class to organize threads
//Thread compass;                                                 // Thread for compass
Thread velocity;                                                // Thread for velocity

//----------------------------------------
Thread imuRead;
Thread forwardSafety;
Thread backwardSafety;
Thread leftSideSafety;
Thread rightSideSafety;
Thread ledgeSafety;
//---------------------------------------

Thread accel_timing;


Thread emergencyButton;                                         // Thread to check button state and reset device
Wheelchair smart(xDir,yDir, &pc, &IMU_t, &wheel, &wheelS, ToFT, &e_button);    // Initialize wheelchair object

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
    
    //queue.call_every(20, &smart, &Wheelchair::compass_thread);        // Sets up sampling frequency of the compass thread
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::velocity_thread);         // Sets up sampling frequency of the velocity thread

    //----------------------------------------------------------------
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::imuRead_thread);
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::forwardSafety_thread);
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::backwardSafety_thread); 
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::leftSideSafety_thread); 
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::rightSideSafety_thread);
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::ledgeSafety_thread);
    //queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::accel_timing);
    //---------------------------------------------------------------------      

    //queue.call_every(200, rosCom_thread);                                     // Sets up sampling frequency of the ROS com thread
    queue.call_every(SAMPLEFREQ, &smart, &Wheelchair::emergencyButton_thread);  // Sets up sampling frequency of the emergency button thread
    
    t.reset();
    //compass.start(callback(&queue, &EventQueue::dispatch_forever));           // Starts running the compass thread
    velocity.start(callback(&queue, &EventQueue::dispatch_forever));            // Starts running the velocity thread

    //-------------------------------------------------------------
    imuRead.start(callback(&queue, &EventQueue::dispatch_forever));
    forwardSafety.start(callback(&queue, &EventQueue::dispatch_forever));
    leftSideSafety.start(callback(&queue, &EventQueue::dispatch_forever));
    rightSideSafety.start(callback(&queue, &EventQueue::dispatch_forever));
    backwardSafety.start(callback(&queue, &EventQueue::dispatch_forever)); 
    ledgeSafety.start(callback(&queue, &EventQueue::dispatch_forever));        
    //---------------------------------------------------------------
    //accel_timing.start(callback(&queue, &EventQueue::dispatch_forever));  

    // Starts running the ROS com thread
    //ros_com.start(callback(&queue, &EventQueue::dispatch_forever));           // Starts running the ROS com thread
    emergencyButton.start(callback(&queue, &EventQueue::dispatch_forever));     // Starts running the emergency button thread
    
    pc.printf("After Starting\r\n");

    //Watchdog dog;                                                               // Creates Watchdog object
    //dog.Configure(watchdogLimit);                                               // Configures timeout for Watchdog
    pc.printf("Code initiated\n");
    int set = 0;
    
    while(1) {

        if( pc.readable()) {
            set = 1;
            char c = pc.getc();                                                 // Read the instruction sent
            if( c == 'e') {
                smart.forward();                                                // Move forward

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
            else if (c == 'p'){
            	smart.desk();
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
        //dog.Service();                                                          // Service the Watchdog so it does not cause a system reset - "Kicking"/"Feeding" the

    }
}

