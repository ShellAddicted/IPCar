#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#ifndef BATTERY_H
#define BATTERY_H
class Battery{
	public:
	Battery(unsigned int emptyBatVoltage=500, unsigned int fullBatVoltage=3300,
			unsigned int vRef = 1100, adc1_channel_t adc1Channel = ADC1_CHANNEL_4,
			adc_atten_t attenuation = ADC_ATTEN_DB_6,
			unsigned int voltageDividerR1 = 0, unsigned int voltageDividerR2=0,
			unsigned int thresholdBatteryConnected = 600);
	~Battery();

	unsigned int getAdcRaw();
	unsigned int getAdcVoltage();

	unsigned int getVoltage();
	uint8_t getPercentage();

	protected:
	adc1_channel_t channel;
	adc_atten_t attenuation;
	unsigned int lastReadVoltage = 0;
	unsigned int lastReadRaw = 0;
	unsigned int voltageDividerR1 = 0;
	unsigned int voltageDividerR2 = 0;
	esp_adc_cal_characteristics_t *adc_chars;
	unsigned int emptyBatVoltage = 0;
	unsigned int fullBatVoltage = 0;
	unsigned int thresholdBatteryConnected = 0;
};
#endif