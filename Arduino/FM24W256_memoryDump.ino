#include "FM24W256.h"

const uint16_t memoryBytesLength = 32768; 
const uint8_t memi2cAddress = 0x0; // Ensure this matches your hardware address
TVA_FRAM fram(memoryBytesLength, memi2cAddress); 

uint16_t currentAddress = 0;
uint16_t sessionCRC = 0; // Global to track CRC across the session

// CRC-16/XMODEM calculation
uint16_t calculateCRC(uint16_t crc, uint8_t data) {
  crc ^= (uint16_t)data << 8;
  for (int i = 0; i < 8; i++) {
    if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
    else crc <<= 1;
  }
  return crc;
}

void sendToSerial(uint8_t data) {
  Serial.write(data);
  currentAddress++;
}

uint8_t readFromSerial() {
  while (Serial.available() == 0); // Wait for data
  uint8_t data = Serial.read();
  currentAddress++;
  return data;
}

// Wrapper for Sending + CRC
void sendToSerialWithCRC(uint8_t data) {
  Serial.write(data);
  sessionCRC = calculateCRC(sessionCRC, data);
  currentAddress++;
}

// Wrapper for Reading + CRC
uint8_t readFromSerialWithCRC() {
  while (Serial.available() == 0); // Wait for data
  uint8_t data = Serial.read();
  sessionCRC = calculateCRC(sessionCRC, data);
  currentAddress++;
  return data;
}

void setup() {
  Serial.begin(9600);
  Serial.println("System booted!");
  Serial.println("Send 'D' to dump memory or 'F' to flash memory.");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 'D') { // DUMP command
      currentAddress = 0;
      sessionCRC = 0; // Reset CRC for new session
      
      fram.StreamRead(0, memoryBytesLength, sendToSerialWithCRC);
      
      // CRITICAL: Wait for the last data byte to actually leave the TX buffer
      Serial.flush(); 
      delay(50); // Small gap to help Python distinguish data from CRC
      
      // Send the 16-bit CRC
      Serial.write((uint8_t)(sessionCRC >> 8));
      Serial.write((uint8_t)(sessionCRC & 0xFF));
      
      // CRITICAL: Wait again before sending text
      Serial.flush();
      delay(50);
      Serial.println("\n[DUMP_FINISHED]"); 
    } 
    else if (cmd == 'F') { // FLASH command
      currentAddress = 0;
      sessionCRC = 0; 
      
      // Tell Python we are ready to receive the binary stream
      Serial.println("[READY_TO_RECEIVE]"); 
      
      fram.StreamWrite(0, memoryBytesLength, readFromSerialWithCRC);
      
      // Read the 2-byte CRC sent by the PC
      uint16_t expectedCRC = (uint16_t)readFromSerial() << 8;
      expectedCRC |= readFromSerial();
      
      if (sessionCRC == expectedCRC) {
        Serial.println("CRC MATCH: Flash Success.");
      } else {
        Serial.print("CRC ERROR: Expected ");
        Serial.print(expectedCRC, HEX);
        Serial.print(" but got ");
        Serial.println(sessionCRC, HEX);
      }
      Serial.println("Memory flash complete.");
    }
  }
}
