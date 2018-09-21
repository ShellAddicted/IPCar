#ifndef VEHICLE_H
#define VEHICLE_H

#include "VehicleAPI.h"

#include <cmath>
#include "Adafruit_MotorShield.h"
#include "BNO055ESP32.h"
#include "Battery.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "servoControl.h"

#define BIT_ERR_I2C_CFG (1 << 0)
#define BIT_ERR_I2C_INSTALL (1 << 1)
#define BIT_ERR_AFMS_INIT_FAIL (1 << 2)
#define BIT_ERR_AFMS_CALL_FAIL (1 << 3)
#define BIT_ERR_IMU_READ_FAIL (1 << 4)
#define BIT_ERR_IMU_INIT_FAIL (1 << 5)

class MyVehicle : public VehicleAPI {
   protected:
    Adafruit_MotorShield afms;
    Adafruit_DCMotor *engine;
    servoControl steeringEngine;

    BNO055 *imu;

    Battery *bat;
    void setupAFMS();
    void setupIMU();

   public:
    MyVehicle();
    void run(direction_t direction, unsigned int throttle, float steering);
    imu_data_t getIMU();
    battery_t getBattery();
};
#endif
