# VATT guide box software

This is the new VATT guidebox software repository. This software comes in three pieces:
1. Control software flashed onto the guide box motors. It is called [allmotors.sms](https://github.com/so-mops/vatt-guidebox/blob/master/allmotors.sms).
2. The [set of functons](https://github.com/so-mops/vatt-guidebox/blob/master/gb_serial.c) used to communicate with the motors
3. The INDI [driver ](https://github.com/so-mops/vatt-guidebox/blob/master/gb_indi.c)
There is also a python module to communicate directly with the motors for testing purposes. 

The motors communicate with the outside world via an RS485 serial line connected to the head node. 




### Control Software (allmotors.sms)
The control software (allmotors.sms) runs on the new Moog Animatics Class 5 M style SmartMotors. It was written using the [SmartMotor Interface software](https://www.animatics.com/products/software/smi-smartmotor-interface). It is a basic-like language that has a very simple and easy to learn syntax. For simplicity and interchangeablitly each motor has the same version of firmware flashed onto it. The program differentiates its behavior based on the CAN address of the motor. This is very important because if a motor is placed is given an incorrect address it can cause damage. For language specifics and anything else you might want to know about the control software refer the [the developers guide ](https://www.animatics.com/downloads/top%20level/4.%20Manuals/c.%20Programming%20Information%20and%20Command%20Reference/SmartMotor%20Developers%20Guide.pdf)

The control program is broken into subroutines that are labeled with C<NUM> where <NUM> is an integer used to name the subroutine. You call the subroutines with the GOSUB(<NUM>) command. When a motor starts up it waits for the head node to call its C1 subroutine. When the head node is started, it waits for the user to call its C0 subroutine. C0 puts in place all the necessary settings like homing position and accel/decel rates. The head node is defined as the motor connected with the RS485 serial line. 
  
  

### Motor Communication Software (gb_serial.c)
This module facilitates the low level communication with the motors. It has convenience functions for anything one would want to do with the guidebox. This includes subroutine calls, initialization, homing, goto and telemetry streaming. 


### INDI Driver (gb_serial.c)
The INDI driver is built with [indilib](https://indilib.org/) though it uses an older C based version of the library and not the C++ version built by Jasem Mutlaq. 



### Current Implementation at VATT
The user interface for this software is the [INDI webclient](https://github.com/srswinde/indi_webclient). The webclient displays the INDI vector properties from the gb_indi.c in a web page. There is a VATT4K version, a VATTSPEC version and an  engineering version. The INDI driver and indi client are started at the system boot using an enabled systemd service. You can see the [Unit file here](https://github.com/srswinde/indi_webclient/blob/master/systemd/webclient-compose.service). You can start, stop or reload this service using the service command or the systemctl command. 

The systemd service uses docker-compose to run three different docker contianers:
1. The nginx webserver
2. The webclient
2. The indiserver and driver

The docker-compose yml file is [here](https://github.com/srswinde/indi_webclient/blob/master/docker-compose-vatt-guidebox.yml). One could of course run all these items outside of docker but docker simplifies the implementation. Docker can however make development a little more complicated as you have to put the new indi-vatt-guidebox binary inside the [indihex](https://hub.docker.com/r/srswinde/indihex) docker contianer. If you wish to run the indidriver natively you can kill the indihex container and run the driver on port 7623 and all should work. 

**Note: this driver runs inside the indihex docker to combine the drivers under one indiserver. Otherwise we would need separate instances of indiserver to run the secondary hexapod and this indi driver.**

### User Bits
User Bits are user defined bits on word 12 of the SmartMotor set of information bits. The allmotors.sms defines them thusly:


| User Bit | Description    | Read Command | Which motors   |
|:--------:|----------------|--------------|----------------|
| Bit 0    | Is Homed       | RB(12,0)     | All Motors     |
| Bit 1    | Motor 1 OK     | RB(12,1)     | Head Node      |
| Bit 2    | Motor 2 OK     | RB(12,2)     | Head Node      |
| Bit 3    | Motor 3 OK     | RB(12,3)     | Head Node      |
| Bit 4    | Motor 4 OK     | RB(12,4)     | Head Node      |
| Bit 5    | Motor 5 OK     | RB(12,5)     | Head Node      |
| Bit 6    | Motor 6 OK     | RB(12,6)     | Head Node      |
| Bit 7    | Motor 7 OK     | RB(12,7)     | Head Node      |
| Bit 8    | Fwheel Moving  | RB(12,8)     | FWHEEL_UPPER   |
| Bit 12   | Motor Ready    | RB(12,12)    | All Motors     |


### IO definitions and other motor specefics:

### OFFSET_X (CAN ADDRESS 1)

The Offset X motor uses an LED and IR sensor to home. The IR light is blocked from the sensor for the entirety of travel except for a small hole that allows light through at the home position. 

When it is not moving a brake is applied to keep it from being backdriven. 

| IO Bit | Description        | Read Command | Direction |
|:------:|--------------------|--------------|-----------|
| Bit 0  | Home Sensor Status | RB(16,0)     | Input     |
| Bit 4  | Homing LED Power   | RB(16,4)     | Output    |
| Bit 5  | Homing LED Power   | RB(16,5)     | Output    |
| Bit 11 | Not Faulted        | RB(16,11)     | Output   |
| Bit 12 | Drive Enabled      | RB(16,12)     | Output   |

### OFFSET_Y (CAN ADDRESS 2)

The Offset Y motor uses an LED and IR sensor to home. The IR light is blocked from the sensor for the entirety of travel except for a small hole that allows light through at the home position. 

When it is not moving a brake is applied to keep it from being backdriven. 

| IO Bit | Description        | Read Command | Direction |
|:------:|--------------------|--------------|-----------|
| Bit 0  | Home Sensor Status | RB(16,0)     | Input     |
| Bit 4  | Homing LED Power   | RB(16,4)     | Output    |
| Bit 5  | Homing LED Power   | RB(16,5)     | Output    |
| Bit 11 | Not Faulted        | RB(16,11)    | Output    |
| Bit 12 | Drive Enabled      | RB(16,12)    | Output    |

### OFFSET_FOCUS (CAN ADDRESS 3)
The offset focus motor has no LED homing sensor it instead homes with the limit switches.

When it is not moving a brake is applied to keep it from being backdriven. 

| IO Bit | Description        | Read Command | Direction |
|:------:|--------------------|--------------|-----------|
| Bit 11 | Not Faulted        | RB(16,11)    | Output    |
| Bit 12 | Drive Enabled      | RB(16,12)    | Input     |


### OFFSET_MIRRORS (CAN ADDRESS 4)
The Offset Mirrors motor uses an LED and IR sensor to home. The IR light is allowed to reach the sensor for most of the travel except for a small tab at one end. The homing is done when the light goes from detect to undetected (edge detection). If light is not detected at the beginning of a homing sequence, the motor moves to the negative limit and then moves positive beyone the light blocking tab. Then it begins the homing sequenc again. 

When it is not moving a brake is applied to keep it from being backdriven. 

| IO Bit | Description        | Read Command | Direction |
|:------:|--------------------|--------------|-----------|
| Bit 0  | Home Sensor Status | RB(16,0)     | Input     |
| Bit 4  | Homing LED Power   | RB(16,4)     | Output    |
| Bit 5  | Homing LED Power   | RB(16,5)     | Output    |
| Bit 11 | Not Faulted        | RB(16,11)    | Output    |
| Bit 12 | Drive Enabled      | RB(16,12)    | Input     |


### OFFSET_FWHEEL (CAN ADDRESS 5)
The Offset Fwheel motor uses two LEDs and two IR sensor to home. The light for the bother sensors is blocked through out most of the travel. There is only one position where both sensor can see the LEDs. This is the home position

When it is not moving a brake is applied to keep it from being backdriven. 

| IO Bit | Description         | Read Command | Direction |
|:------:|-------------------- |--------------|-----------|
| Bit 0  | Home Sensor Status  | RB(16,0)     | Input     |
| Bit 1  | Filter Sensor Status| RB(16,0)     | Input     |
| Bit 4  | Homing LED Power    | RB(16,4)     | Output    |
| Bit 5  | Homing LED Power    | RB(16,5)     | Output    |
| Bit 11 | Not Faulted         | RB(16,11)    | Output    |
| Bit 12 | Drive Enabled       | RB(16,12)    | Input     |


### LOWER_FWHEEL (CAN ADDRESS 6)
The Lower Fwheel motor uses two LEDs and two IR sensor to home. The light for the bother sensors is blocked through out most of the travel. There is only one position where both sensor can see the LEDs. This is the home position. 

When the Lower Fwheel moves it must retract a solenoid that is controlled by the Upper Fwheel. The solenoid holds both the upper and lower filter wheels in place while their motor is not moving them. 

| IO Bit | Description         | Read Command | Direction |
|:------:|-------------------- |--------------|-----------|
| Bit 0  | Home Sensor Status  | RB(16,0)     | Input     |
| Bit 1  | Filter Sensor Status| RB(16,0)     | Input     |
| Bit 4  | Homing LED Power    | RB(16,4)     | Output    |
| Bit 5  | Homing LED Power    | RB(16,5)     | Output    |
| Bit 11 | Not Faulted         | RB(16,11)    | Output    |
| Bit 12 | Drive Enabled       | RB(16,12)    | Input     |


### UPPER_FWHEEL (CAN ADDRESS 7)
The Upper Fwheel motor uses two LEDs and two IR sensors to home. The light for the bother sensors is blocked through out most of the travel. There is only one position where both sensor can see the LEDs. This is the home position. 

When the Upper Fwheel moves it must retract a solenoid that it controls. The solenoid holds both the upper and lower filter wheels in place while their motor is not moving them. You will notice there are more IO definitions on this motor. This is because of the solenoid control. 

| IO Bit | Description          | Read Command | Direction |
|:------:|--------------------  |--------------|-----------|
| Bit 0  | Home Sensor Status   | RB(16,0)     | Input     |
| Bit 1  | Filter Sensor Status | RB(16,0)     | Input     |
| Bit 4  | Homing LED Power     | RB(16,4)     | Output    |
| Bit 5  | Homing LED Power     | RB(16,5)     | Output    |
| Bit 7  | Solenoid Sensor Power| RB(16,7)     | Output    |
| Bit 8  | Solenoid Sensor Power| RB(16,8)     | Output    |
| Bit 9  | Retract Solenoid     | RB(16,9)     | Output    |
| Bit 11 | Not Faulted          | RB(16,11)    | Output    |
| Bit 12 | Drive Enabled        | RB(16,12)    | Input     |
