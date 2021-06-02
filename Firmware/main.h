#pragma once

#include "stm32f030f4p6.h"
#include "at250x0b.h"
#include "cc1101.h"

constexpr auto using_version = corelib::version { 0, 1, corelib::version::dev_status::alpha, 5 };
static_assert(corelib::current_version == using_version, "version of attached library mismatches the desired version of the library.");

constexpr uint32_t ahb_clock = 8000000;

auto& modules_en = gpio::PA1;
auto& led        = alternative_pins::usart1_tx.pin;
auto& cc1101_cs  = gpio::PA3;
auto& eeprom_cs  = gpio::PA4;
auto& cold_pin   = gpio::PA9;
auto& hot_pin    = gpio::PA10;

auto& cold_interrupt = exti9;
auto& hot_interrupt = exti10;
auto& counters_interrupt_handler = exti_4_to_15;
#define COUNTERS_INTERRUPT EXTI_4_TO_15

struct wcsm
{
	constexpr static uint8_t header[] = { 11, 22, 33, 44, 55, 66, 77, 88 };
	uint16_t adc_value = 0;
	uint32_t cold = 0;
	uint32_t hot = 0;
	void serialize(uint8_t* data, size_t& length) const
	{
		for (size_t i = 0; i < sizeof(header); i++)
			data[i] = header[i];

		uint16_t* p_adc = reinterpret_cast<uint16_t*>(data + sizeof(header));
		uint32_t* p_cold = reinterpret_cast<uint32_t*>(p_adc + 1);
		uint32_t* p_hot = p_cold + 1;

		*p_adc = adc_value;
		*p_cold = cold;
		*p_hot = hot;

		length = sizeof(header) + sizeof(adc_value) + sizeof(cold) + sizeof(hot);
	}
};

constexpr const gpio::basic_bits allpins = 
	gpio::pins::_0.m_bits |
	gpio::pins::_1.m_bits |
	gpio::pins::_2.m_bits |
	gpio::pins::_3.m_bits |
	gpio::pins::_4.m_bits |
	gpio::pins::_5.m_bits |
	gpio::pins::_6.m_bits |
	gpio::pins::_7.m_bits |
	gpio::pins::_8.m_bits |
	gpio::pins::_9.m_bits |
	gpio::pins::_10.m_bits |
	gpio::pins::_11.m_bits |
	gpio::pins::_12.m_bits |
	gpio::pins::_13.m_bits |
	gpio::pins::_14.m_bits |
	gpio::pins::_15.m_bits ;

// also configures swd, so flashing the chip can be impossible
// gpio::port_A.configure(allpins, gpio::mode::input_pull_up);
// gpio::port_B.configure(allpins, gpio::mode::input_pull_up);
