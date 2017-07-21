/*
 * ds1302.h
 *
 * Created: 2017-07-21 20:12:00
 * Author : Krisztian Stancz
 *
 * Library for ATmega168 family controllers to access and manipulate
 * DS1302 Real Time Clock devices. Communication is done 
 * via a 3-wire interface
 *
 * USAGE:
 * The most convenient way of reading or setting the time is
 * by creating an rtc_time_t structure and using:
 * - rtc_get_time()
 * - rtc_set_time()
 *
 * The read_rtc() and write_rtc() functions are also available to
 * access specific RTC registers. See data_t definition for the available
 * registers. NOTE: the user must ensure data integrity. Also before setting
 * the HOUR values, it is important to properly set the HOUR_FORMAT.
 *
 * The functions read_rtc_burst() and write_rtc_burst() are provided to
 * access the entire RTC or RAM register set in one operation. However 
 * no value translation is done, the user must need to take care of it.
 * Please refer to the DS1302 documentation for register definitions.
 *
 */ 
#ifndef __DS1302_H_INCLUDED
#define __DS1302_H_INCLUDED

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

// Update value according to processor type
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>

/*
 * CLK - SCLK - PC0
 * DAT - I/O  - PC1
 * RST - CE   - PC2
 */

#define PSCLK	0
#define PIO		1
#define PCE		2

#define SCLK_PORT	PORTC
#define IO_PORT		PORTC
#define CE_PORT		PORTC

#define SCLK_DDR	DDRC
#define IO_DDR		DDRC
#define CE_DDR		DDRC

#define IO_PIN		PINC

#define MAX_RTC_BYTES	9
#define MAX_RAM_BYTES	31

typedef enum {SEC, MIN, HOUR, DATE, MONTH, DAY, YEAR, AM_PM,
	          HOUR_FORMAT, CLOCK_HALT, WRITE_PROTECT, HOUR_REG,
			  TRICKLE_CHARGE_REG
} data_t;

typedef enum {RTC, RAM} reg_t;
typedef enum {AM, PM} tformat_t;
typedef enum {H24, H12} hformat_t;
	
typedef struct {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t date;	// Day of the month
	uint8_t month;
	uint8_t year;
	uint8_t day;	// Day of the week
	hformat_t hour_format;
	tformat_t am_pm;
} rtc_time_t;

// Reads RTC data into an rtc_time_t data structure
uint8_t rtc_get_time(rtc_time_t* time_data);

// Updates the time stored in RTC from an rtc_time_t data structure
int8_t rtc_set_time(rtc_time_t* time_data);

// Reads a single time data from the RTC
// Accepts data_t type and returns decimal value
uint8_t read_rtc(data_t type);

// Writes a single time data into the RTC
// Accepts data_t type and a decimal value
void write_rtc(data_t type, uint8_t data);

// Reads the contents of the RTC or RAM registers
// in a single burst into the provided buffer
void read_rtc_burst(reg_t registers, uint8_t* data_buff);

// Writes the contents of the provided buffer 
// into the RTC or RAM registers in a single burst
void write_rtc_burst(reg_t registers, uint8_t* data_buff);

// Reads a single byte of data from RTC
// Used by higher level functions
uint8_t read_byte(uint8_t command_byte);

// Writes a single byte of data into the RTC
// Used by higher level functions
void write_byte(uint8_t command_byte, uint8_t data_byte);

// Reads 8 bits from the RTC
// Used by higher level functions
uint8_t read_bits(void);

// Writes 8 bits into the RTC
// Used by higher level functions
void write_bits(uint8_t byte);

// Initializes RTC pins for read/write operations
void init_pins(void);

#endif // __DS1302_H_INCLUDED