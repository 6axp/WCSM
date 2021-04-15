#include "main.h"

volatile void delay(unsigned ticks = 200000) { delay_ticks(ticks); }

auto& unused_pin = gpio::PB1;

auto& modules_en = gpio::PA1;
auto& led        = alternative_pins::usart1_tx.pin;
auto& cc1101_cs  = gpio::PA3;
auto& eeprom_cs  = gpio::PA4;
auto& meter_in1  = gpio::PA9;
auto& meter_in2  = gpio::PA10;

struct wcsm_data
{
	constexpr static uint8_t header[] = { 11, 22, 33, 44, 55, 66, 77, 88 };
	uint16_t adc_value = 0;
	uint32_t cold = 0;
	uint32_t hot = 0;
	bool is_ok = false;
	void serialize(uint8_t* data, size_t& length) const
	{
		for (size_t i = 0; i < sizeof(header); i++)
			data[i] = header[i];

		uint16_t* pAdc = reinterpret_cast<uint16_t*>(data + sizeof(header));
		uint32_t* pCold = reinterpret_cast<uint32_t*>(pAdc + 1);
		uint32_t* pHot = pCold + 1;

		*pAdc = adc_value;
		*pCold = cold;
		*pHot = hot;

		length = sizeof(header) + sizeof(adc_value) + sizeof(cold) + sizeof(hot);
	}
};

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

void prepare_for_sleep()
{
	// achieves 0.046 mA

	power.clock.disable();
	spi1.disable();
	USART1->CR1 &= ~(USART_CR1_RE | USART_CR1_TE);
    USART1->CR1 &= ~USART_CR1_UE;
	ADC1->CR |= ADC_CR_ADDIS;
	adc1.m_rcc.disable();
	modules_en.low();
	RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
	spi1.m_rcc.disable();
	power.clock.disable();

	unused_pin.configure(gpio::mode::analog_input); // not connected
	gpio::ports::A.configure(allpins, gpio::mode::analog_input); // TODO: don't configure PA13 & PA14 pins (SWDIO & SWCLK)

	battery.gpio_pin.configure(gpio::mode::analog_input); // connected to resistor divider
	modules_en.configure(gpio::mode::analog_input); // pulled to GND

	eeprom_cs.configure(gpio::mode::analog_input);
	cc1101_cs.configure(gpio::mode::analog_input);
	meter_in1.configure(gpio::mode::input_pull_up);
	meter_in2.configure(gpio::mode::input_pull_up);

	spi1.m_miso.pin.configure(gpio::mode::input_pull_down);
	spi1.m_mosi.pin.configure(gpio::mode::input_pull_down);
	alternative_pins::usart1_tx.pin.configure(gpio::mode::input_pull_up);

	gpio::ports::A.clock.disable();
	gpio::ports::B.clock.disable();
}

int blinks = 0;

uint16_t cold_counter = 100;
uint16_t hot_counter = 200;

int main(void)
{
	delay(3000000);

	modules_en.configure(gpio::mode::output);
	modules_en.high();
	usart1.configure(8000000, 115200);

	at250x0b eeprom(spi1, eeprom_cs, true, true);
	usart1.send("-------");
	usart1.println();
	eeprom.print_to(usart1);

	power.clock.enable();
	power.sleep_after_interrupts(false);

	battery.configure();

	meter_in1.configure(gpio::mode::input_pull_up);
	meter_in2.configure(gpio::mode::input_pull_up);

	cc1101_class cc1101(spi1, cc1101_cs);
	cc1101.configure();

	usart1.send("cc verison: ");
	usart1.sendln(cc1101.get_version());

	cc1101.configure();
	cc1101.set_cc_mode();
	cc1101.set_modulation(modulation_mode::_2_FSK);
	cc1101.set_frequency(frequency::_433);
	cc1101.set_output_power(12);
	cc1101.set_sync_mode(2);
	cc1101.enable_crc();

	//delay(1000000);

	//usart1.send("going to sleep\r\n");
	
	//cc1101.send_command(strobe_commands::SXOFF);

	exti9.configure(interrupts::trigger::falling);
	exti10.configure(interrupts::trigger::falling);
	exti_4_to_15.enable();

	while (true)
	{
		delay(1000000);
		uint8_t to_send[256];
		size_t to_send_len = 0;
		wcsm_data {
			battery.get_adc(10),
			cold_counter,
			hot_counter
		}.serialize(to_send, to_send_len);

		cc1101.send_data(to_send, to_send_len);
		led.configure(gpio::mode::output);
		for (int i = 0; i < blinks; i++)
		{
			led.low();
			delay_ticks(300000);
			led.high();
			delay_ticks(300000);
		}
		blinks = 0;
		//prepare_for_sleep();
		//power.stop(pwr::regulator_mode::low_power);
	}
}

IRQ_HANDLER(EXTI_4_TO_15)
{
	if (exti9)
	{
		exti9.reset();
		blinks = 1;
		hot_counter++;
	}

	if (exti10)
	{
		exti10.reset();
		blinks = 2;
		cold_counter++;
	}
}
