import sys
import serial
from intelhex import IntelHex

ser = serial.Serial()
ser.baudrate = 62500
ser.port     = '/dev/ttyUSB0'
ser.stopbits = serial.STOPBITS_ONE
ser.xonoff   = 0
ser.rtscts   = 0
ser.timeout  = 1

ser.open()

def write_bytes(address, bytes):
    count = len(bytes)
    cmd = f'B{address:0{4}x}{count:0{4}x}'
    ser.write(cmd.encode())
    ser.flush()
    echo = ser.read(len(cmd)).decode()
    if echo != cmd:
        return None
    ser.write(bytes)
    ser.flush()
    return list(ser.read(count))

hex = IntelHex(sys.argv[1])

print('READY')

def chunks(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

fails = 0
for (start, stop) in hex.segments():
    for chunk in chunks(range(start, stop), 1024):
        address = chunk[0]
        data = [hex[a] for a in chunk]
        print(f'WRITING {len(data)} BYTES AT ADDRESS ${address:0{4}x}')
        while True:
            check = write_bytes(address, data)
            if check == data:
                break
            if check is None:
                print(f"--- COMMAND ERROR - RETRYING ---")
            elif len(check) != len(data):
                print(f"--- LENGTH ERROR ({len(check)}) - RETRYING ---")
            else:
                print(f"--- VERIFICATION ERROR - RETRYING ---")

            fails+=1

ser.close()

print(f'DONE, {fails} RETRIES')
