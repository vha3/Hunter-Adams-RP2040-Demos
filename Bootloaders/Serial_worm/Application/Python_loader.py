import serial
import time

# Using readlines()
file1 = open('./build/blinky.hex', 'r')
Lines = file1.readlines()

# For IR, use a buad rate of 4800
# Otherwise, use a buad rate of 115200

ser = serial.Serial(
    port='/dev/tty.usbserial-0001',
    baudrate=4800,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=None
)

# Open the serial port
ser.isOpen()

# Send some hexlines with bad checksums to get into known bootloader state
for i in range(10):
    ser.write(":020000041000EB\r".encode()) ;
    time.sleep(0.1)
    ser.read(1)

# We've accumulated junk in the UART input buffer, clear it
ser.reset_input_buffer()

# Send one final bad checksum, which puts a single char in our
# UART input buffer
ser.write(":020000041000EB\r".encode()) ;
time.sleep(0.1)

# How many lines to we have to send? How many have we sent?
counter = 1
total_lines = float(len(Lines))

# For each line in the hexfile . . .
for line in Lines:

    # Keep track of progress
    percent_finished = int((counter/total_lines)*100)
    counter += 1

    # This only returns when we receive a character. It'll either be A or B
    # For as long as it's A, loop here sending the same line
    while(ord(ser.read(1))==65):

        # Report our progress
        print(" Progress: " + str(percent_finished) + " percent ", end="\r")

        ## Write the line
        ser.write(line.encode())


    # # Via IR
    # for i in line:
    #     ser.write(i.encode())
    #     time.sleep(0.025)
    # # time.sleep(0.1) ;

# let's wait one second before reading output (let's give device time to answer)
time.sleep(1)
while ser.inWaiting() > 0:
    print(ser.read(1))
ser.close()