#include "PcmTrack.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

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
	return 0;
}
