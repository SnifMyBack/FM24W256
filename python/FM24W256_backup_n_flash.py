import serial
import time

# Configuration
PORT = 'COM5' 
BAUD = 9600
FILENAME = "fram_backup.bin"
SIZE = 32768

def crc16_xmodem(data: bytes):
    crc = 0
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000: crc = (crc << 1) ^ 0x1021
            else: crc <<= 1
        crc &= 0xFFFF
    return crc

def backup():
    with serial.Serial(PORT, BAUD, timeout=5) as ser:
        time.sleep(2)
        print("Waiting for Arduino menu...")
        
        # Clear startup garbage
        ser.reset_input_buffer()
        ser.write(b'D')
        
        print("Receiving data...")
        # 1. Read EXACTLY the binary data
        raw_data = b''
        while len(raw_data) < SIZE:
            chunk = ser.read(SIZE - len(raw_data))
            if not chunk: break
            raw_data += chunk
            print(f"Progress: {len(raw_data)}/{SIZE}", end='\r')

        # 2. Read the CRC (sent after the delay)
        print("\nVerifying Checksum...")
        crc_bytes = ser.read(2)
        if len(crc_bytes) < 2:
            print("Error: Did not receive CRC bytes from Arduino.")
            return

        received_crc = int.from_bytes(crc_bytes, 'big')
        calculated_crc = crc16_xmodem(raw_data)

        if calculated_crc == received_crc:
            with open(FILENAME, "wb") as f: f.write(raw_data)
            print(f"SUCCESS! CRC {hex(calculated_crc)} matches.")
        else:
            print(f"FAILED! Local CRC: {hex(calculated_crc)}, Arduino: {hex(received_crc)}")
        
        # Print any remaining text from Arduino
        print("Arduino Final Message:", ser.read_all().decode('utf-8', errors='ignore').strip())

def restore():
    if not input("Confirm Restore? (y/n): ").lower() == 'y': return
    
    with open(FILENAME, "rb") as f: data = f.read()
    my_crc = crc16_xmodem(data)

    with serial.Serial(PORT, BAUD, timeout=5) as ser:
        time.sleep(2)
        ser.write(b'F')
        time.sleep(0.5)
        
        print("Uploading...")
        ser.write(data)
        
        print("Sending Checksum...")
        time.sleep(0.1) # Small gap before CRC
        ser.write(my_crc.to_bytes(2, 'big'))
        
        # Wait for the result text
        result = ser.read_until(b"Memory flash complete.").decode('utf-8', errors='ignore')
        print(f"Arduino Result: {result.strip()}")

def test_memory_bits():
    with serial.Serial(PORT, BAUD, timeout=5) as ser:
        print("--- Starting Memory Bit Test ---")
    
        patterns = [0xAA, 0x55]
        
        for pattern in patterns:
            time.sleep(2)
            print("Waiting for Arduino's answer...")
            
            # Clear startup garbage
            ser.reset_input_buffer()

            pattern_hex = hex(pattern)
            print(f"\nPhase: Testing pattern {pattern_hex}...")
            
            # 1. FLASH Phase
            ser.write(b'F')
            
            # WAIT for the Arduino to be inside the StreamWrite loop
            ready_signal = ser.readline().decode().strip()
            if "[READY_TO_RECEIVE]" not in ready_signal:
                print(f"Error: Arduino not ready. Got: {ready_signal}")
                return

            print(f"Uploading {pattern_hex}...")
            test_data = bytes([pattern]) * SIZE
            ser.write(test_data)
            
            # Small sleep before sending CRC to let Arduino catch up
            time.sleep(0.5) 
            pattern_crc = crc16_xmodem(test_data)
            ser.write(pattern_crc.to_bytes(2, 'big'))
            
            # Wait for Arduino result
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line: print(f"Arduino: {line}")
                if "flash complete" in line.lower(): break
                if "error" in line.lower(): return

            # 2. VERIFY Phase
            print(f"Verifying {pattern_hex}...")
            ser.write(b'D')
            
            received_data = b''
            # Read exactly the SIZE
            while len(received_data) < SIZE:
                chunk = ser.read(SIZE - len(received_data))
                if not chunk: break
                received_data += chunk
                
            received_crc_bytes = ser.read(2)
            received_crc = int.from_bytes(received_crc_bytes, 'big')
            
            if crc16_xmodem(received_data) != received_crc:
                print(f"FAIL: CRC mismatch. Expected {hex(crc16_xmodem(received_data))}, got {hex(received_crc)}")
                return
            
            # Final byte-by-byte check
            if all(b == pattern for b in received_data):
                print(f"SUCCESS: Pattern {pattern_hex} verified perfectly.")
            else:
                print(f"FAILED: Bits did not match the pattern in memory.")
                return

        print("\n--- ALL BIT TESTS PASSED ---")

if __name__ == "__main__":
    op = input("Action (B)ackup, (T)est bits or (R)estore: ").upper()
    if op == 'B': backup()
    elif op == 'R': restore()
    elif op == 'T': test_memory_bits()
    else: print("Invalid option. Use B for Backup, R for Restore, T for Test.")
