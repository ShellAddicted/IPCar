#include "Battery.h"

Battery::Battery(unsigned int emptyBatVoltage, unsigned int fullBatVoltage, unsigned int vRef, adc1_channel_t adc1Channel, adc_atten_t attenuation, unsigned int voltageDividerR1, unsigned int voltageDividerR2, unsigned int thresholdBatteryConnected){
	this->emptyBatVoltage = emptyBatVoltage;
	this->fullBatVoltage = fullBatVoltage;
	this->channel = adc1Channel;
	this->attenuation = attenuation;
	this->voltageDividerR1 = voltageDividerR1;
	this->voltageDividerR2 = voltageDividerR2;
	this->thresholdBatteryConnected = thresholdBatteryConnected;

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten((adc1_channel_t)channel, attenuation);
	this->adc_chars = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, attenuation, ADC_WIDTH_BIT_12, vRef, adc_chars);
}

Battery::~Battery(){
	free(this->adc_chars);
}

unsigned int Battery::getAdcRaw(){
	unsigned int raw = 0;
	//Multisampling
	for (int i = 0; i < 64; i++) {
		raw += adc1_get_raw((adc1_channel_t)channel);
	}
	raw /= 64;
	return raw;
}

unsigned int Battery::getAdcVoltage(){
	unsigned int raw = getAdcRaw();
	return esp_adc_cal_raw_to_voltage(raw, adc_chars);
}

unsigned int Battery::getVoltage(){
	unsigned int voltage = getAdcVoltage();
	if (voltage < thresholdBatteryConnected){
		return 0; // Bat is not Connected.
	}
	else if (voltageDividerR1 == 0 && voltageDividerR2 == 0){
		// NO Voltage devider
		return voltage;
	}
	else{
		// There's a voltage divider
		return voltage*(voltageDividerR1+voltageDividerR2)/voltageDividerR2;
	}
}

uint8_t Battery::getPercentage(){
	unsigned int v = getVoltage();
	double percentage = 100 * ((double)(v - emptyBatVoltage) / (fullBatVoltage - emptyBatVoltage));
	if (percentage < 0){
		return 0;
	}
	else if (percentage > 100){
		return 100;
	}
	return (uint8_t) percentage;
}