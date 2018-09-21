// Vehicle Specific API

#include "MyVehicle.h"

float getValueByPercentage(float percentage, float total, bool protection){
	if (protection == true && std::abs(percentage) > 100){
		percentage = 100;
	}
	else if (protection == true && std::abs(percentage) <= 0){
		return 0;
	}
	return ((float)percentage/100)*total;
}

const char *MyVehicle::logTag = "VehicleAPI";

MyVehicle::MyVehicle(){
	// Attach Sterring Servo
	steeringEngine.attach(GPIO_NUM_23);

	// Setup I²C bus 
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = GPIO_NUM_21;
	conf.scl_io_num = GPIO_NUM_22;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	i2c_param_config(I2C_NUM_0, &conf);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	
	//turn ON AFMS PWM Generator
	gpio_pad_select_gpio(GPIO_NUM_4);
	gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 1);
	
	//AFMS
	afms = Adafruit_MotorShield(0x60, I2C_NUM_0);
	engine = afms.getMotor(2);
	try{
		afms.begin();
	}
	catch (std::exception& exc){
		ESP_LOGE(MyVehicle::logTag, "AFMS init failed. Exception: %s", exc.what());
	}

	//BNO055
	bno055_offsets_t storedOffsets;
	storedOffsets.accelOffsetX = 29;
	storedOffsets.accelOffsetY = 24;
	storedOffsets.accelOffsetZ = 16;
	storedOffsets.magOffsetX = -243;
	storedOffsets.magOffsetY = -420;
	storedOffsets.magOffsetZ = -131;
	storedOffsets.gyroOffsetX = 1;
	storedOffsets.gyroOffsetY = -1;
	storedOffsets.gyroOffsetZ = 0;
	storedOffsets.accelRadius = 0;
	storedOffsets.magRadius = 662;
    try{
		imu = new BNO055(UART_NUM_1, GPIO_NUM_16, GPIO_NUM_17);
        imu->begin();
        imu->enableExternalCrystal();
        imu->setSensorOffsets(storedOffsets);
        imu->setOprModeNdof();
		imuReady = true;
    }
    catch (std::exception& ex){
        ESP_LOGE(MyVehicle::logTag, "Setup Failed, Error: %s", ex.what());
    }

	//Battery
	bat = new Battery(5500, 8400, 1063, ADC1_CHANNEL_4, ADC_ATTEN_DB_6, 4190, 994, 600);
	startTelemetryTask();
}

void MyVehicle::run(direction_t direction, unsigned int throttle, float steering){
	VehicleAPI::run(direction,throttle,steering);
	//Steering
	float servoAngle = 100; // Servo Angle: Center
	if (steering > 0){ // left
		servoAngle += getValueByPercentage(steering, 15, true); 
	}
	else if (steering < 0){ // right
		servoAngle += getValueByPercentage(steering, 35, true); // 23 instead of 20 due to compensation.
	}
	steeringEngine.write((int)servoAngle);
	
	//Throttle
	int pwm = getValueByPercentage(throttle, 255, true);
	if (direction == Directions::Brake){ //Braking is not actually supported
		engine->run((uint8_t)Directions::Release);
	}
	else{
		//Send Direction && speed to the engine
		engine->setSpeed(pwm);
		engine->run((uint8_t)direction);
	}
}

imu_data_t MyVehicle::getIMU(){
	imu_data_t data;
	bno055_vector_t tmp;
	try{
		if (imuReady){
			tmp = imu->getVectorLinearAccel();
			data.lia.x = tmp.x;
			data.lia.y = tmp.y;
			data.lia.z = tmp.z;

			tmp = imu->getVectorEuler();
			data.euler.x = tmp.x;
			data.euler.y = tmp.y;
			data.euler.z = tmp.z;

			data.orientation_mode = IMUOrientationModes::Relative;
			data.calibration = (imu_calibration_t) imu->getCalibration().sys;
			data.success = true;
		}
		else{
			data.success = false;
		}
	}
	catch(std::exception &e){
		data.success = false;
	}
	return data;
}

battery_t MyVehicle::getBattery(){
	battery_t bt;
	bt.voltage = bat->getVoltage();
	bt.percentage = bat->getPercentage();
	bt.state = BatteryStates::discharging;
	bt.success = true;
	return bt;
}