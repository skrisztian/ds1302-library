/*
 * ds1302.c
 *
 * Created: 2017-07-21 20:12:00
 * Author : Krisztian Stancz
 */ 

#include "ds1302.h"

void init_pins(void)
{
	// Set all pins to output
	SCLK_DDR |= 1 << PSCLK;
	IO_DDR |= 1 << PIO;
	CE_DDR |= 1 << PCE;
	
	// Reset all pins
	SCLK_PORT &= ~(1 << PSCLK);
	IO_PORT &= ~(1 << PIO);
	CE_PORT &= ~(1 << PCE);
}

uint8_t read_byte(uint8_t command_byte)
{
	uint8_t data_byte = 0;
	init_pins();
	
	// Set CE to HIGH
	CE_PORT |= 1 << PCE;
	
	// Wait 4 us
	_delay_us(4);

	// First 8 bits is sending command byte.
	write_bits(command_byte);
	
	// Set IO to input and reset pin state
	IO_DDR &= ~(1 << PIO);
	IO_PORT &= ~(1 << PIO);
	
	// Second 8 bits is reading the answer.
	data_byte = read_bits();
	
	// Set CE to LOW
	CE_PORT &= ~(1 << PCE);
	
	return data_byte;
}

void write_byte(uint8_t command_byte, uint8_t data_byte)
{
	init_pins();
	
	// Set CE to HIGH
	CE_PORT |= 1 << PCE;
	
	// Wait 4 us
	_delay_us(4);

	// First 8 bits is sending command byte.
	write_bits(command_byte);
	
	// Reset IO pin
	IO_PORT &= ~(1 << PIO);
	
	// Second 8 bits is setting the register.
	write_bits(data_byte);
	
	// Set CE to LOW
	CE_PORT &= ~(1 << PCE);
	
	return;
}

void write_bits(uint8_t byte)
{
	// Data must be present on clock rising edge.
	for (int i = 0; i < 8; i++) {
	
		// Set data
		if (byte & (1 << i))
			IO_PORT |= 1 << PIO;
		else
			IO_PORT &= ~(1 << PIO);
	
		// Clock rising edge
		SCLK_PORT |= 1 << PSCLK;
	
		// Clock falling edge
		SCLK_PORT &= ~(1 << PSCLK);
	}

	return;
}

uint8_t read_bits(void)
{
	uint8_t data_byte = 0;
	
	// Data comes in on falling edge.
	for (int i = 0; i < 8; i++) {

		// Clock rising edge
		SCLK_PORT |= 1 << PSCLK;
		
		// Clock falling edge
		SCLK_PORT &= ~(1 << PSCLK);
		
		// Read data
		uint8_t data_bit = (IO_PIN & (1 << PIO)) ? 1 : 0;
		data_byte |= data_bit << i;
	}
	
	return data_byte;
}
	
void read_rtc_burst(reg_t registers, uint8_t* data_buff)
{
	uint8_t max_bytes = 0;
	switch (registers) {
	case RTC:
		max_bytes = MAX_RTC_BYTES;
		break;
	case RAM:
		max_bytes = MAX_RAM_BYTES;
		break;
	}
	
	init_pins();
	
	// Set CE to HIGH
	CE_PORT |= 1 << PCE;
	
	// Wait 4 us
	_delay_us(4);

	// First 8 bits is sending read burst command byte.
	write_bits(0xbf);
	
	// Set IO to input and reset pin state
	IO_DDR &= ~(1 << PIO);
	IO_PORT &= ~(1 << PIO);
	
	// Read the answer in 8 bit packages
	for (int j = 0; j < max_bytes; j++) {
		data_buff[j] = read_bits();
	}
	
	// Set CE to LOW
	CE_PORT &= ~(1 << PCE);
	
	return;
}

void write_rtc_burst(reg_t registers, uint8_t* data_buff)
{
	uint8_t max_bytes = 0;
	switch (registers) {
	case RTC:
		max_bytes = MAX_RTC_BYTES;
		break;
	case RAM:
		max_bytes = MAX_RAM_BYTES;
		break;
	}
	
	init_pins();
	
	// Set CE to HIGH
	CE_PORT |= 1 << PCE;
	
	// Wait 4 us
	_delay_us(4);

	// First 8 bits is sending write burst command byte
	write_bits(0xbe);
	
	// Reset IO pin state
	IO_PORT &= ~(1 << PIO);
	
	// Clear write protect
	write_rtc(WRITE_PROTECT, 0);
	
	// Write into the registers in 8 bit packages
	for (int j = 0; j < max_bytes; j++) {
		write_bits(data_buff[j]);
	}
	
	// Set write protect
	write_rtc(WRITE_PROTECT, 1);
	
	
	// Set CE to LOW
	CE_PORT &= ~(1 << PCE);
	
	return;
}

uint8_t read_rtc(data_t type)
{
	uint8_t temp = 0;
	uint8_t value = 0;
	
	switch (type) {
	case SEC:
		temp = read_byte(0x81);
		value = 10 * ((temp & 0x70) >> 4) + (temp & 0x0f);
		break;
	case MIN:
		temp = read_byte(0x83);
		value = 10 * ((temp & 0x70) >> 4) + (temp & 0x0f);
		break;
	case HOUR:
		temp = read_byte(0x85);
		if (((temp & 0x80) ? H12 : H24) == H24)
			// 24 hour format
			value = 10 * ((temp & 0x30) >> 4) + (temp & 0x0f);
		else
			// 12 hour format
			value = 10 * ((temp & 0x10) >> 4) + (temp & 0x0f);
		break;
	case DATE:
		temp = read_byte(0x87);
		value = 10 * ((temp & 0x30) >> 4) + (temp & 0x0f);
		break;
	case MONTH:
		temp = read_byte(0x89);
		value = 10 * ((temp & 0x10) >> 4) + (temp & 0x0f);
		break;
	case DAY:
		value = read_byte(0x8b);
		break;
	case YEAR:
		temp = read_byte(0x8d);
		value = 10 * ((temp & 0xf0) >> 4) + (temp & 0x0f);
		break;
	case AM_PM:
		value = (read_byte(0x85) & 0x20) ? PM : AM;
		break;
	case HOUR_FORMAT:
		value = (read_byte(0x85) & 0x80) ? H12 : H24;
		break;
	case CLOCK_HALT:
		value = (read_byte(0x81) & 0x80) ? 1 : 0;
		break;
	case WRITE_PROTECT:
		value = (read_byte(0x8f) & 0x80) ? 1 : 0;
		break;
	case HOUR_REG:
		value = read_byte(0x85);
		break;
	case TRICKLE_CHARGE_REG:
		value = read_byte(0x91);
	break;
	}
	
	return value;
}

void write_rtc(data_t type, uint8_t data)
{
	uint8_t temp;
	uint8_t value;
	
	// Clear write protect
	if (type != WRITE_PROTECT)
		write_rtc(WRITE_PROTECT, 0);
	
	switch (type) {
	case SEC:
		value = ((data / 10) << 4) | (data % 10);
		write_byte(0x80, value);
		break;
	case MIN:
		value = ((data / 10) << 4) | (data % 10);
		write_byte(0x82, value);
		break;
	case HOUR:
		// Convert value into the currently used time format
		temp = read_byte(0x85);
		uint8_t time_format_12 = temp & 0x80;
		if (time_format_12) {
			// 12 hour format
			if (data > 12)
				data -= 12;
			uint8_t pm = temp & 0x20;
			value = ((data / 10) << 4) | (data % 10) | time_format_12 | pm;
			write_byte(0x84, value);
		} else {
			// 24 hour format
			value = ((data / 10) << 4) | (data % 10);
			write_byte(0x84, value);
		}
		break;
	case DATE:
		value = ((data / 10) << 4) | (data % 10);
		write_byte(0x86, value);
		break;
	case MONTH:
		value = ((data / 10) << 4) | (data % 10);
		write_byte(0x88, value);
		break;
	case DAY:
		write_byte(0x8a, data);
		break;
	case YEAR:
		value = ((data / 10) << 4) | (data % 10);
		write_byte(0x8c, value);
		break;
	case AM_PM:
		// Only set AM/PM if 12 hour format is used
		temp = read_rtc(HOUR_REG);
		if(((temp & 0x80) ? H12 : H24) == H12) {
			if (data)
				value = temp | (1 << 5);
			else
				value = temp & ~(1 << 5);
			write_byte(0x84, value);
		}
		break;
	case HOUR_FORMAT:
		temp = read_rtc(HOUR_REG);
		uint8_t pm = AM;
		uint8_t hour = 0;
		if (data && !((temp & 0x80) ? H12 : H24)) {
			// Currently 24h, setting to 12h
			// Get current time
			hour = 10 * ((temp & 0x30) >> 4) + (temp & 0x0f);
			// Convert time to 12h format
			if (hour > 12) {
				hour -= 12;
				pm = PM;
			}
			// Set 10 hours + hours + 12h format + AM/PM 
			value = ((hour / 10) << 4) | (hour % 10) | (data << 7) | (pm << 5);
			write_byte(0x84, value);
		} else if (!data && ((temp & 0x80) ? H12 : H24)) {
			// Currently 12h setting to 24h
			// Get current time and AM/PM setting
			hour = 10 * ((temp & 0x10) >> 4) + (temp & 0x0f);
			pm = (temp & 0x20) ? PM : AM;
			// Set 10 hours + hours. 24h format is 0 on the 7th bit
			value = (((hour + pm * 12) / 10) << 4) | ((hour + pm * 12) % 10);
			write_byte(0x84, value);
		}
		break;
	case CLOCK_HALT:
		temp = read_rtc(SEC);
		value = ((temp / 10) << 4) | (temp % 10) | (data << 7);
		write_byte(0x80, value);
		break;
	case WRITE_PROTECT:
		write_byte(0x8e, data << 7);
		break;
	case HOUR_REG:
		write_byte(0x84, data);
		break;
	case TRICKLE_CHARGE_REG:
		write_byte(0x90, data);
		break;
	}
		
	// Set write protect
	if (type != WRITE_PROTECT)
		write_rtc(WRITE_PROTECT, 1);

	return;
}

uint8_t rtc_get_time(rtc_time_t* time_data)
{
	time_data->sec = read_rtc(SEC);
	time_data->min = read_rtc(MIN);
	time_data->hour = read_rtc(HOUR);
	time_data->date = read_rtc(DATE);
	time_data->month = read_rtc(MONTH);
	time_data->year = read_rtc(YEAR);
	time_data->day = read_rtc(DAY);
	time_data->hour_format = read_rtc(HOUR_FORMAT);
	
	// Only makes sense if hour format is H12
	time_data->am_pm = read_rtc(AM_PM);

	return 0;
}

int8_t rtc_set_time(rtc_time_t* time_data)
{
	// Validate time_data values
	
	if (time_data->hour_format != H12 &&
		time_data->hour_format != H24 )
		return -1;
		
	if (time_data->sec > 59)
		return -2;
		
	if (time_data->min > 59)
		return -3;
		
	if ((time_data->hour_format == H12 && time_data->hour > 12) || 
		(time_data->hour > 23))
		return -4;
		
	if (time_data->date > 31)
		return -5;

	if (time_data->month > 12)
		return -6;
		
	if (time_data->year > 99)
		return -7;
		
	if (time_data->day > 7)
		return -8;
	
	if ((time_data->hour_format == H12) && 
	    !((time_data->am_pm == AM) || (time_data->am_pm == PM)))
		return -9;
	
	// Make sure hour format is set before hour value
	write_rtc(HOUR_FORMAT, time_data->hour_format);

	write_rtc(SEC, time_data->sec);
	write_rtc(MIN, time_data->min);
	write_rtc(HOUR, time_data->hour);
	write_rtc(DATE, time_data->date);
	write_rtc(MONTH, time_data->month);
	write_rtc(YEAR, time_data->year);
	write_rtc(DAY, time_data->day);
	
	// Only set AM/PM if hour format is H12
	if (time_data->hour_format == H12)
		write_rtc(AM_PM, time_data->am_pm);

	return 0;
}

