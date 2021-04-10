#include "main.h"

volatile void delay(unsigned ticks = 200000) { delay_ticks(ticks); }

auto& unused_pin = gpio::PB1;

auto& modules_en = gpio::PA1;
auto& led        = alternative_pins::usart1_tx.pin;
auto& cc1101_cs  = gpio::PA3;
auto& eeprom_cs  = gpio::PA4;
auto& meter_in1  = gpio::PA9;
auto& meter_in2  = gpio::PA10;

constexpr struct
{
	const float vref = 3.0f;
	const float divider_ratio = 2.258f;
	const gpio::pin& gpio_pin = gpio::PA0;
	const adc::pin& adc_pin = adc::PA0;
	const uint16_t upper_bound = (1 << 12) - 1;
	void configure() const { adc_pin.m_adc.configure(); }
	float get_voltage(uint32_t measures = 1) const
	{
		adc_pin.m_adc.select_channel(adc_pin.m_channel);
		uint16_t sum = 0;
		for (uint16_t i = 0; i < measures; i++)
			sum += adc_pin.m_adc.measure();

		return (sum / measures) * divider_ratio * vref / upper_bound;
	}
} battery;

void prepare_for_sleep()
{
	unused_pin.configure(gpio::mode::input_pull_up); // not connected
	gpio::port_A.configure(allpins, gpio::mode::input_pull_up);

	battery.gpio_pin.configure(gpio::mode::floating_input); // connected to resistor divider
	modules_en.configure(gpio::mode::floating_input); // pulled to GND
	led.configure(gpio::mode::input_pull_up);
	eeprom_cs.configure(gpio::mode::input_pull_up);
	// cc1101_cs.configure(gpio::mode::floating_input); // not connected
	// meter_in1.configure(gpio::mode::floating_input); // not connected
	// meter_in2.configure(gpio::mode::floating_input); // not connected
}

int main(void)
{
	modules_en.configure(gpio::mode::output);
	modules_en.high();
	usart1.configure(8000000, 115200);
	battery.configure();

	at250x0b eeprom(spi1, eeprom_cs, true, true);
	usart1.send("-------");
	usart1.println();
	eeprom.print_to(usart1);

	while (true)
	{
		usart1.sendln(battery.get_voltage(10));
		delay(1000000);
	}
}
