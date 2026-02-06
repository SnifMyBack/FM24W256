#include <stdlib.h>
#include <Wire.h>
#include "FM24W256.h"

TVA_FRAM::TVA_FRAM(uint16_t maxMemAddr, uint8_t device_addr)
{
	Wire.begin();
  this->maxMemAddr = maxMemAddr;  // 32KB for FM24W256
  this->device_addr = device_addr | 0x50; // I2C address of the FM24W256 FRAM 0x50 to 0x57 depending on A0, A1, A2 pins (0x50 = 0b1010000)
}

uint32_t TVA_FRAM::eraseDevice(void)
{
  uint32_t startmillis;
  uint32_t endmillis;
  startmillis = millis();
  uint16_t i = 0;
  Serial.println("Start erasing device");
  while ((i < maxMemAddr)) {
    WriteByte(i, 0x00);
    i++;
  }
  endmillis = millis();
  return endmillis - startmillis;
}

void TVA_FRAM::WriteString(uint16_t Start_Add, String theString, uint16_t sizeofarray)
{
  uint8_t data[sizeofarray];
  theString.getBytes(data, sizeofarray);
  for (int i = 0; i < sizeofarray - 1 ; i++)
  {
    WriteByte(Start_Add + i, data[i]);
    Serial.println(i);
  }
  Serial.println();
}

void TVA_FRAM::WriteString(uint16_t Start_Add, const uint8_t theStringArray[], uint16_t sizeofarray)
{
  for (int i = 0; i < sizeofarray - 1; i++)
  {
    WriteByte(Start_Add + i, theStringArray[i]);
  }
}

void TVA_FRAM::WriteByte(uint16_t address, uint8_t data)
{
  Wire.beginTransmission(this->device_addr);
  Wire.write((uint8_t)highByte(address));
  Wire.write((uint8_t)lowByte(address));
  Wire.write(data);
  Wire.endTransmission();
}

int16_t TVA_FRAM::ReadByte(uint16_t address)
{
  Wire.beginTransmission(this->device_addr);
  Wire.write((uint8_t)highByte(address));
  Wire.write((uint8_t)lowByte(address));
  Wire.endTransmission();
  
  Wire.requestFrom(this->device_addr, (uint8_t)1);
  
  if (Wire.available())
  {
    return Wire.read();
  }
  return -1;
}

// 1. Tell the FRAM which address we want to start at
int8_t TVA_FRAM::BeginRead(uint16_t address) {
  Wire.beginTransmission(this->device_addr);
  Wire.write((uint8_t)highByte(address));
  Wire.write((uint8_t)lowByte(address));
  int8_t status = Wire.endTransmission();
  
  if (status != 0) {
    // 1: Data too long
    // 2: NACK on transmit of address (Chip didn't respond)
    // 3: NACK on transmit of data (Chip rejected the MSB/LSB)
    // 4: Other error
    return -status; // Return the negative error code
  }
  return 0; // Success
}

// 2. Request a chunk and stream it
// You call this in a loop; it returns -1 when no more data is available
int16_t TVA_FRAM::GetNextByte() {
  if (Wire.available()) {
    return Wire.read();
  }
  return -1; 
}

void TVA_FRAM::StreamRead(uint16_t address, uint16_t size, void (*callback)(uint8_t)) {
  const uint16_t MAX_WIRE_SIZE = 32;
  uint16_t bytesRead = 0;
  
  while (bytesRead < size) {
    uint16_t bytesToRead = (size - bytesRead > MAX_WIRE_SIZE) ? MAX_WIRE_SIZE : (size - bytesRead);
    
    Wire.beginTransmission(this->device_addr);
    Wire.write((uint8_t)((address + bytesRead) >> 8));
    Wire.write((uint8_t)((address + bytesRead) & 0xFF));
    Wire.endTransmission(false);
    
    Wire.requestFrom(this->device_addr, (uint8_t)bytesToRead);
    
    while (Wire.available()) {
      callback(Wire.read());
      bytesRead++;
    }
  }
}

/**
 * @brief Streams data directly from Serial to FRAM in 30-byte chunks.
 * @param startAddress The memory address to begin writing.
 * @param totalSize    Total bytes expected from Serial.
 */
void TVA_FRAM::StreamWrite(uint16_t startAddress, uint32_t totalSize, uint8_t (*callback)(void)) {
  uint32_t bytesWritten = 0;
  uint16_t currentAddress = startAddress;

  while (bytesWritten < totalSize) {
    // Start a new I2C transaction for a chunk
    Wire.beginTransmission(this->device_addr);
    Wire.write((uint8_t)(currentAddress >> 8));   // MSB
    Wire.write((uint8_t)(currentAddress & 0xFF)); // LSB

    uint8_t chunkCounter = 0;
    // Fill the I2C buffer (max 30 bytes to stay within Wire's 32-byte limit)
    while (chunkCounter < 30 && bytesWritten < totalSize) {
      if (Serial.available() > 0) {
        Wire.write(callback());
        chunkCounter++;
        bytesWritten++;
      }
      // Small timeout/yield could be added here if needed for high-speed Serial
    }

    // Push the chunk to the physical FRAM
    Wire.endTransmission();
    
    // Increment memory address by the amount of data actually sent
    currentAddress += chunkCounter;
  }
}