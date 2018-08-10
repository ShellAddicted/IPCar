#ifndef VEHICLE_H
#define VEHICLE_H

#include "VehicleAPI.h"

#include <cmath>
#include "esp_log.h"
#include "BNO055ESP32.h"
#include "servoControl.h"
#include "Adafruit_MotorShield.h"
#include "Battery.h"

class MyVehicle: public VehicleAPI{
	protected:

	static const char *logTag;
	
	Adafruit_MotorShield afms;
	Adafruit_DCMotor *engine;
	servoControl steeringEngine;

	BNO055* imu;
	bool imuReady = false;

	Battery* bat;

	public:

	MyVehicle();
	void run(direction_t direction, unsigned int throttle, float steering);
    imu_data_t getIMU();
	battery_t getBattery();
};
#endif
