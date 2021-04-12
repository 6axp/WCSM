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
	// achieves 0.04 mA

	unused_pin.configure(gpio::mode::analog_input); // not connected
	gpio::port_A.configure(allpins, gpio::mode::analog_input);

	battery.gpio_pin.configure(gpio::mode::analog_input); // connected to resistor divider
	modules_en.configure(gpio::mode::analog_input); // pulled to GND

	eeprom_cs.configure(gpio::mode::analog_input);
	cc1101_cs.configure(gpio::mode::analog_input);
	meter_in1.configure(gpio::mode::input_pull_up);
	meter_in2.configure(gpio::mode::input_pull_up);

	spi1.m_miso.pin.configure(gpio::mode::input_pull_down);
	spi1.m_mosi.pin.configure(gpio::mode::input_pull_down);
	alternative_pins::usart1_tx.pin.configure(gpio::mode::input_pull_up);
}

int main(void)
{
	delay(3000000);

	prepare_for_sleep();
	power.stop(pwr::regulator_mode::low_power);
	while (true);
	
	modules_en.configure(gpio::mode::output);
	modules_en.high();
	usart1.configure(8000000, 115200);

	at250x0b eeprom(spi1, eeprom_cs, true, true);
	usart1.send("-------");
	usart1.println();
	eeprom.print_to(usart1);

	power.clock.enable();
	power.sleep_after_interrupts(false);

	modules_en.low();

	battery.configure();

	meter_in1.configure(gpio::mode::input_pull_up);
	meter_in2.configure(gpio::mode::input_pull_up);
	exti9.configure(interrupts::trigger::falling);
	exti10.configure(interrupts::trigger::falling);
	exti_4_to_15.enable();

	while (true)
	{
		//usart1.configure(8000000, 115200);
		//battery.configure();
		usart1.sendln(battery.get_voltage(10));
		power.stop();

		//RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
		//battery.adc_pin.m_adc.m_rcc.disable();
		//power.sleep_until_interrupt();

		//prepare_for_sleep();
		//power.sleep_until_interrupt();
		//battery.configure();
		//usart1.configure(8000000, 115200);
		//usart1.sendln(battery.get_voltage(10));
	}
}

IRQ_HANDLER(EXTI_4_TO_15)
{
	if (exti9)
		exti9.reset();

	if (exti10)
		exti10.reset();


}
