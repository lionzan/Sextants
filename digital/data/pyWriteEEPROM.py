
#######################################################
#
#  simple txt-bin converter for Stars Data to be loaded
#  on an EEPROM for the Sextant project
#
#  written by @lionzan, 2019-03-02
#
#  referece https://stackoverflow.com/questions/18367007/python-how-to-write-to-a-binary-file
#   
#######################################################

import struct

output = []
items = []

fileIn = open('StarsData.csv', 'r')
for row in fileIn.read().split('\n'):
    items.append(row.split(','))

fileOut = open('StarsData1.bin', 'wb')
for item in items:
    c3=bytes(item[3],'utf-8')
    c4=bytes(item[4],'utf-8')
    a=struct.pack('fff6s13s',float(item[0]),float(item[1]),float(item[2]),c3,c4)
    fileOut.write(a)
    print(a)
    print('\n')

fileIn.close()
fileOut.close()
    
    output.append(a)

for out in output:
    fileOut.write(out)
fileOut.close()
