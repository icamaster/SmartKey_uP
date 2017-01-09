import serial
import sys

serBlue = serial.Serial('COM7', 9600, timeout=2000)
serVirtual = serial.Serial('COM14', 9600, timeout=2000)

while True:
  try:
    tempString = serBlue.readline();
    serVirtual.write(tempString);

  except KeyboardInterrupt:
    print "Exiting!"
    serBlue.close()
    serVirtual.close()
    break