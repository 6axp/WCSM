#pragma once

struct wcsm
{
	constexpr static uint8_t header[] = { '6', 'a', 'x', 'p', 'w', 'c', 's', 'm' };
	uint32_t adc = 0;
	uint32_t cold = 0;
	uint32_t hot = 0;

    static constexpr size_t get_packet_size()
    {
        return (sizeof(header) * sizeof(*header)) + sizeof(adc) + sizeof(cold) + sizeof(hot);
    }

	void serialize(uint8_t* data, size_t& length) const
	{
		for (size_t i = 0; i < sizeof(header); i++)
			data[i] = header[i];

		uint32_t* p_adc = reinterpret_cast<uint32_t*>(data + sizeof(header));
		uint32_t* p_cold = p_adc + 1;
		uint32_t* p_hot = p_cold + 1;

		*p_adc = adc;
		*p_cold = cold;
		*p_hot = hot;
		length = get_packet_size();
	}

    bool deserialize(uint8_t* data, size_t length)
    {
		if (length != get_packet_size())
			return false;

		for (size_t i = 0; i < sizeof(header); i++)
			if (header[i] != data[i])
				return false;

		uint32_t* p_adc = reinterpret_cast<uint32_t*>(data + sizeof(header));
		uint32_t* p_cold = p_adc + 1;
		uint32_t* p_hot = p_cold + 1;

		adc = *p_adc;
		cold = *p_cold;
		hot = *p_hot;
        return true;
    }
};
