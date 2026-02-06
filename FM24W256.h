#ifndef _FM24W256_H_
#define _FM24W256_H_

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include <Wire.h>

class TVA_FRAM {
	public:
		TVA_FRAM(uint16_t maxMemAddr, uint8_t device_addr);
        uint32_t eraseDevice(void);
        void WriteString(uint16_t Start_Add, String theString, uint16_t sizeofarray);
        void WriteString(uint16_t Start_Add, const uint8_t theStringArray[], uint16_t sizeofarray);
        void WriteByte(uint16_t address, uint8_t data);
        int16_t ReadByte(uint16_t address);
        int8_t BeginRead(uint16_t address);
        int16_t GetNextByte();
        void StreamRead(uint16_t address, uint16_t size, void (*callback)(uint8_t));
        void StreamWrite(uint16_t startAddress, uint32_t totalSize, uint8_t (*callback)(void));

	private:
		uint16_t maxMemAddr;
		uint8_t device_addr;
};

#endif
