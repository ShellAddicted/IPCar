#ifndef VEHICLE_API_H
#define VEHICLE_API_H

#include <exception>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TELEMETRY_RECORDS_SIZE 20
typedef enum class Directions {
    Backward = 1,
    Forward,
    Brake,
    Release,
    MAX
} direction_t;

typedef enum class IMUOrientationModes {
    Relative = 1,
    Abslute,
    MAX
} imu_orientation_mode_t;

typedef enum class IMUCalibration {
    NO = 0,
    Low,
    Medium,
    High,
    MAX
} imu_calibration_t;

typedef struct {
    double x = 0;
    double y = 0;
    double z = 0;
} imu_vector_t;

typedef struct {
    bool success = true;
    imu_orientation_mode_t orientation_mode;
    imu_vector_t euler;
    imu_vector_t lia;
    imu_calibration_t calibration;
} imu_data_t;

typedef struct {
    bool success = false;
    unsigned long rpm = 0;
    double speed = 0;
    double distance = 0;
    double diameter = 0;
} wheel_t;

typedef enum class BatteryStates {
    charging = 0,
    discharging,
    MAX
} battery_state_t;

typedef struct {
    bool success = false;
    unsigned long voltage = 0;
    double percentage = 0;
    battery_state_t state;
} battery_t;

typedef struct {
    int errorCode;
    unsigned long uptime;

    // Wheels
    wheel_t wheels[4];

    // Engine
    direction_t direction;
    unsigned int throttle;

    // Steering
    float steering;

    // IMU
    imu_data_t imu;

    // Battery
    battery_t battery;
} telemetry_data_t;

class VehicleAPI {
   protected:
    EventGroupHandle_t errorFlags = NULL;
    float currentSteering = 0;  // %
    direction_t currentEngineDirection = Directions::Release;
    unsigned int currentEngineThrottle = 0;  // %
    static direction_t getOppositeDirection(direction_t Direction);
    virtual void run(direction_t direction, unsigned int throttle, float steering);
    virtual telemetry_data_t genTelemetry();
    static void telemetryTask(void* VehiclePtr);
    void startTelemetryTask();

   public:
    VehicleAPI();
    ~VehicleAPI();

    QueueHandle_t telemetry_records = NULL;

    float getCurrentSteering();
    direction_t getCurrentEngineDirection();
    unsigned int getCurrentEngineThrottle();
    EventBits_t getErrorCode();

    virtual void getWheels(wheel_t* wheels, unsigned int len);
    virtual imu_data_t getIMU();
    virtual battery_t getBattery();
};

#endif