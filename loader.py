import sys
import serial
from intelhex import IntelHex

ser = serial.Serial()
ser.baudrate = 62500
ser.port     = '/dev/ttyUSB0'
ser.stopbits = serial.STOPBITS_ONE
ser.xonoff   = 0

ser.open()

def set_address(address):
    cmd = f'A{address:0{4}x}'
    #print('\n'+cmd)
    ser.write(cmd.encode())
    assert ser.read(len(cmd)).decode() == cmd

def read_bytes(count):
    for _ in range(count):
        ser.write(b'Rxx')
    result=[]
    for _ in range(count):
        assert ser.read(1) == b'R'
        data = int(ser.read(2), 16)
        #print(f'R{data:0{2}x}', end=' ')
        result.append(data)
    return result

def write_bytes(bytes):
    for data in bytes:
        cmd = f'W{data:0{2}x}'
        #print(cmd, end=' ')
        ser.write(cmd.encode())
    for data in bytes:
        cmd = f'W{data:0{2}x}'
        assert ser.read(len(cmd)).decode() == cmd

hex = IntelHex(sys.argv[1])

print('READY', end='')

def chunks(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

fails = 0
for (start, stop) in hex.segments():
    for chunk in chunks(range(start, stop), 64):
        address = chunk[0]
        data = [hex[a] for a in chunk]
        print(f'WRITING {len(data)} BYTES AT ADDRESS ${address:0{4}x}')
        while True:
            set_address(address)
            write_bytes(data)
            set_address(address)
            check = read_bytes(len(data))
            if check == data:
                break
            print("\n--- VERIFICATION ERROR - RETRYING ---", end="")
            fails+=1

ser.close()

print(f'\nDONE, {fails} RETRIES')