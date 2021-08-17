#include "main.h"

cc1101_class cc1101(spi1, cc1101_cs);
at250x0b eeprom(spi1, eeprom_cs);

uint16_t counter_address = 0x00;
long usart_baud = 115200;

uint32_t cold_counter = 100;
uint32_t hot_counter = 200;
bool counters_changed = false;
int blinks = 0;

constexpr struct
{
	const float vref = 3.0f;
	const float divider_ratio = 2.258f;
	const gpio::pin& gpio_pin = gpio::PA0;
	const adc::pin& adc_pin = adc::PA0;
	const uint16_t upper_bound = (1 << 12) - 1;
	void configure() const { adc_pin.m_adc.configure(); }
	uint16_t get_adc(size_t measures = 1) const
	{
		adc_pin.m_adc.select_channel(adc_pin.m_channel);
		uint32_t sum = 0;
		for (size_t i = 0; i < measures; i++)
			sum += adc_pin.m_adc.measure();
		return static_cast<uint16_t>(sum / measures);
	}
	float get_voltage(size_t measures = 1) const
	{
		return get_adc(measures) * divider_ratio * vref / upper_bound;
	}
} battery;

void go_to_sleep() // achieves 0.046 mA
{
	power.clock.disable();
	spi1.disable();
	USART1->CR1 &= ~(USART_CR1_RE | USART_CR1_TE | USART_CR1_UE);
	ADC1->CR |= ADC_CR_ADDIS;
	adc1.m_rcc.disable();
	modules_en.low();
	RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
	spi1.m_rcc.disable();
	power.clock.disable();

	gpio::ports::A.configure(allpins, gpio::mode::analog_input); // TODO: don't configure PA13 & PA14 pins (SWDIO & SWCLK)
	gpio::ports::B.configure(allpins, gpio::mode::analog_input);

	battery.gpio_pin.configure(gpio::mode::analog_input); // connected to resistor divider
	modules_en.configure(gpio::mode::analog_input); // pulled to GND

	eeprom_cs.configure(gpio::mode::analog_input);
	cc1101_cs.configure(gpio::mode::analog_input);
	cold_pin.configure(gpio::mode::input_pull_up);
	hot_pin.configure(gpio::mode::input_pull_up);

	spi1.m_miso.pin.configure(gpio::mode::input_pull_down);
	spi1.m_mosi.pin.configure(gpio::mode::input_pull_down);
	alternative_pins::usart1_tx.pin.configure(gpio::mode::input_pull_up);

	gpio::ports::A.clock.disable();
	gpio::ports::B.clock.disable();

	power.stop(pwr::regulator_mode::low_power);
}

void configure()
{
	modules_en.configure(gpio::mode::output);
	modules_en.high();

	usart1.configure(ahb_clock, usart_baud);
	usart1.println("-------");

	battery.configure();

	cc1101_cs.configure(gpio::mode::output);
	eeprom_cs.configure(gpio::mode::output);
	cc1101_cs.high();
	eeprom_cs.high();

	delay_ticks(100000); // let cc1101 initialize after power up

	eeprom.configure(false);

	cc1101.configure();
	cc1101.set_cc_mode();
	cc1101.set_modulation(modulation_mode::_2_FSK);
	cc1101.set_frequency(frequency::_433);
	cc1101.set_output_power(12);
	cc1101.set_sync_mode(2);
	cc1101.enable_crc();

	usart1.print("cc verison: ");
	usart1.println(cc1101.get_version());

	led.configure(gpio::mode::output);
	led.high();

	power.clock.enable();
	power.sleep_after_interrupts(false);
	cold_pin.configure(gpio::mode::input_pull_up);
	hot_pin.configure(gpio::mode::input_pull_up);
	hot_interrupt.configure(interrupts::trigger::falling);
	cold_interrupt.configure(interrupts::trigger::falling);
	counters_interrupt_handler.enable();
}

void get_counters_from_eeprom(uint32_t& cold, uint32_t& hot)
{
	uint32_t data[2];
	eeprom.read(counter_address, data);
	cold = data[0];
	hot  = data[1];
}

void save_counters_to_eeprom(uint32_t cold, uint32_t hot)
{
	uint32_t data[2] = { cold, hot };
	eeprom.write(counter_address, data);
}

void send_data(uint32_t adc, uint32_t cold, uint32_t hot)
{
	uint8_t to_send[256];
	size_t to_send_len = 0;
	wcsm { adc, cold, hot }.serialize(to_send, to_send_len);
	cc1101.send_data(to_send, to_send_len);

	usart1.configure(ahb_clock, usart_baud);
	usart1.print("cold: ");
	usart1.println((int)cold);
	usart1.print("hot: ");
	usart1.println((int)hot);
}

void blink(size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		led.low();
		delay_ticks(300000);
		led.high();
		delay_ticks(300000);
	}
}

int main(void)
{
	configure();
	get_counters_from_eeprom(cold_counter, hot_counter);
	send_data(battery.get_adc(), cold_counter, hot_counter);
	blink(1);

	while (true)
	{
		if (counters_changed)
		{
			save_counters_to_eeprom(cold_counter, hot_counter);
			send_data(battery.get_adc(), cold_counter, hot_counter);
			counters_changed = false;
		}
		if (blinks)
		{
			led.configure(gpio::mode::output);
			blink(blinks);
			blinks = 0;
		}

		// go_to_sleep();
	}
}

IRQ_HANDLER(COUNTERS_INTERRUPT)
{
	counters_changed = true;

	if (hot_interrupt)
	{
		blinks = 1;
		hot_counter++;
		hot_interrupt.reset();
	}

	if (cold_interrupt)
	{
		blinks = 2;
		cold_counter++;
		cold_interrupt.reset();
	}
}
