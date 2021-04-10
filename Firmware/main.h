#pragma once

#include "D:/Projects/STM32/Libraries/CoreLib/chips/stm32f030f4p6.h"
#include "D:/Projects/STM32/Libraries/Corelib/at250x0b.h"

// power.stop(pwr::regulator_mode::normal); // 0.19 mA
// power.stop(pwr::regulator_mode::low_power); // 0.006 mA
// power.standby(); // 0.0025 mA
// power.sleep(); // 1.03 mA

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
