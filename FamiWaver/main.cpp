#include "PcmTrack.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>


void TestEncode(const uint8_t* wave, size_t size)
{
#if 0
	std::map<std::vector<uint8_t>, int> form_map;
	static const size_t form_len = 9;
	std::vector<uint8_t> form(form_len);
	for (size_t i = 0; i < size - form_len - 1; ++i)
	{
		for (size_t n = 0; n < form_len; ++n)
			form[n] = wave[i+1+n] - wave[i];
		auto itt = form_map.find(form);
		if (itt == form_map.end())
		{
			form_map.insert(std::make_pair(form, 1));
		}
		else
		{
			++itt->second;
		}
	}

	std::vector<std::pair<int, std::vector<uint8_t>>> sort_vector;
	for (auto itt = form_map.begin(); itt != form_map.end(); ++itt)
		sort_vector.push_back(std::make_pair(-itt->second, itt->first));

	std::sort(sort_vector.begin(), sort_vector.end());
	for (int i = 0; i < 32; ++i)
	{
		std::cout << std::dec << i << ":" << -sort_vector[i].first << ":";
		for (size_t n = 0; n < form_len; ++n)
			std::cout << std::hex << std::setw(2) << std::setfill('0') << ((uint16_t)sort_vector[i].second[n]) << " ";
		std::cout << std::endl;
	}
#endif
	size_t avg_count = 0;
	size_t non_avg_count = 0;
	for (size_t i = 0; i < size-4; i += 2)
	{
		if ((wave[i] + wave[i + 2])/2 == wave[i + 1])
			avg_count++;
		else
			non_avg_count++;
	}
	std::cout << "Avg=" << avg_count << " non avg=" << non_avg_count << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		std::cout << "USAGE:" << std::endl;
		std::cout << "  FamiWaver input.wav output samplerate volume" << std::endl;
		std::cout << "    output - file name prefix, also internal prefix before" << std::endl;
		std::cout << "    samplerate - conversion samplerate 1000...48000, default 10000" << std::endl;
		std::cout << "    volume - volume amplify in percentage 1..1000, default 100" << std::endl;
		return -1;
	}

	int samplerate;
	int volume;
	try
	{
		 samplerate = std::stoi(argv[3]);
		 volume = std::stoi(argv[4]);
	}
	catch (const std::exception&)
	{
		std::cout << "Invalid integer" << std::endl;
		return -1;
	}
	
	if (samplerate < 1000 || samplerate > 48000)
	{
		std::cout << "Invalid samplerate value" << std::endl;
		return -1;
	}

	if (volume < 1 || volume > 1000)
	{
		std::cout << "Invalid volume value" << std::endl;
		return -1;
	}

	PcmTrack track;
	if (!track.Load(argv[1]))
		return -1;
	PcmTrack dst;
	if (!track.ConvertTo_16bit(dst))
		return -1;
	track = dst;

	if (!track.ConvertTo_Mono(dst))
		return -1;
	track = dst;

	if (!track.Resample(dst, samplerate))
		return -1;
	track = dst;
	//track.Save("10000.wav");
		
	//track.ConvertTo_8bit_unsigned(dst);
	//dst.Save("8bit.wav");

	track.ConvertTo_7bit(dst, (float)volume / 100.0f);
	//track.ConvertTo_7bit_Amplify(dst);
	{
		std::stringstream stream;
		stream << argv[2] << ".wav";
		if (!dst.Save(stream.str().c_str()))
			return -1;
	}


	TestEncode(dst.m_data.data(), dst.m_data.size());

#if 0
	for (size_t n = 0; n < dst.m_data.size(); n += 8*1024)
	{
		std::stringstream stream;
		stream << argv[2] << "_" << std::dec << std::setw(2) << std::setfill('0') << (n / (1024 * 8)) << ".inc";
		std::ofstream file(stream.str());
		file << argv[2] << "_" << std::dec << std::setw(2) << std::setfill('0') << (n / (1024 * 8)) << ":" << std::endl;
			
		size_t pos = 0;
		for (size_t i = 0; i < 8*1024; ++i)
		{
			if (n + i >= dst.m_data.size())
				break;
			if (pos == 0)
				file << "    .db ";
			else
				file << ", ";
			file << "$" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (uint32_t)dst.m_data[i+n];
			pos++;
			if (pos == 16)
			{
				pos = 0;
				file << std::endl;
			}
		}
	}
#endif
	return 0;
}
