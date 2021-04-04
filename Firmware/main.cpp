#include "main.h"

volatile void delay(unsigned ticks = 200000) { delay_ticks(ticks); }

auto& unused_pin = gpio::PB1;

auto& adc_in     = gpio::PA0;
auto& modules_en = gpio::PA1;
auto& led        = alternative_pins::usart1_tx.pin;
auto& eeprom_cs  = gpio::PA3;
auto& cc1101_cs  = gpio::PA4;
auto& meter_in1  = gpio::PA9;
auto& meter_in2  = gpio::PA10;

void prepare_for_sleep()
{
	unused_pin.configure(gpio::mode::input_pull_up); // not connected
	gpio::port_A.configure(allpins, gpio::mode::input_pull_up);

	adc_in.configure(gpio::mode::floating_input); // connected to resistor divider
	modules_en.configure(gpio::mode::floating_input); // pulled to GND
	led.configure(gpio::mode::input_pull_up);
	eeprom_cs.configure(gpio::mode::input_pull_up);
	// cc1101_cs.configure(gpio::mode::floating_input); // not connected
	// meter_in1.configure(gpio::mode::floating_input); // not connected
	// meter_in2.configure(gpio::mode::floating_input); // not connected
}

int main(void)
{
	//rcc::initialize();
	led.configure(gpio::mode::output);
	int tick_until_sleep = 10;
	while (true)
	{
		delay(400000);
		led.toggle();
		if (tick_until_sleep-- <= 0)
		{
			prepare_for_sleep();
			power.standby();
		}
			
	}
}
