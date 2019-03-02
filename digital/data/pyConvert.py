output = []
items = []
fileIn = open('StarsData.csv', 'r')
for row in fileIn.read().split('\n'):
    items.append[row.split(',')]
for item in items:
    output.append(float(item[0])
    output.append(float(item[1])
    output.append(float(item[2])
    output.append(item[3])
    output.append(item[4])
    
    
from array import array
output_file = open('StarsData.bin', 'wb')
float_array = array('d', [3.14, 2.7, 0.0, -1.0, 1.1])
float_array.tofile(output_file)
output_file.close()

s = bytes(s, 'utf-8')    # Or other appropriate encoding
struct.pack("I%ds" % (len(s),), len(s), s)

//    referece
    https://stackoverflow.com/questions/18367007/python-how-to-write-to-a-binary-file
