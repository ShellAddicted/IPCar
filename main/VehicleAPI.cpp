#include "VehicleAPI.h"

VehicleAPI::VehicleAPI() {
    telemetry_records = xQueueCreate(TELEMETRY_RECORDS_SIZE, sizeof(telemetry_data_t));
    errorFlags = xEventGroupCreate();
}

VehicleAPI::~VehicleAPI() {
    vQueueDelete(telemetry_records);
    vEventGroupDelete(errorFlags);
}

void VehicleAPI::startTelemetryTask() {
    xTaskCreate(VehicleAPI::telemetryTask, "telemetryTask", 4096, this, 5, NULL);
}

direction_t VehicleAPI::getOppositeDirection(direction_t Direction) {
    if (Direction == Directions::Forward) {
        //if vehicle is going Forward set the opposite Direction
        return Directions::Backward;
    } else if (Direction == Directions::Backward) {
        //if vehicle is going Forward set the opposite Direction
        return Directions::Forward;
    } else {
        return Directions::Release;
    }
}

direction_t VehicleAPI::getCurrentEngineDirection() {
    return currentEngineDirection;
}

float VehicleAPI::getCurrentSteering() {
    return currentSteering;
}

unsigned int VehicleAPI::getCurrentEngineThrottle() {
    return currentEngineThrottle;
}

void VehicleAPI::run(direction_t direction, unsigned int throttle, float steering) {
    currentSteering = steering;
    currentEngineThrottle = throttle;
    currentEngineDirection = direction;
}

void VehicleAPI::getWheels(wheel_t* wheels, unsigned int len) {
    for (unsigned int i = 0; i < len; i++) {
        wheels[i].success = false;
    }
}

EventBits_t VehicleAPI::getErrorCode() {
    return xEventGroupGetBits(errorFlags);
}

imu_data_t VehicleAPI::getIMU() {
    imu_data_t data;
    data.success = false;
    return data;
}

battery_t VehicleAPI::getBattery() {
    battery_t bat;
    return bat;
}

telemetry_data_t VehicleAPI::genTelemetry() {
    telemetry_data_t currentRecord;

    currentRecord.uptime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    currentRecord.errorCode = getErrorCode();

    currentRecord.throttle = getCurrentEngineThrottle();
    currentRecord.direction = getCurrentEngineDirection();
    currentRecord.steering = getCurrentSteering();

    currentRecord.imu = getIMU();
    currentRecord.battery = getBattery();
    getWheels(currentRecord.wheels, 4);
    return currentRecord;
}

void VehicleAPI::telemetryTask(void* ptr) {
    VehicleAPI* car = (VehicleAPI*)ptr;

    for (;;) {
        telemetry_data_t currentRecord = car->genTelemetry();
        xQueueSend(car->telemetry_records, (void*)&currentRecord, 0);
        vTaskDelay(pdMS_TO_TICKS(10));  //100hz
    }
    vTaskDelete(NULL);
}
