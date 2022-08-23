#!/bin/env python

import serial
import time
import struct
import subprocess
import binascii


temp_command: str = './get_temp.sh'
memory_command: str = './get_memory.sh'
disk_command: str = './get_disk.sh'

mem_first: str = 'MemTotal:'
mem_second: str = 'MemFree:'
mem_third: str = 'MemAvailable:'
mem_end: str = 'kB'

#CMD_ID_SEND = b'\x72'
#end_of_t = b'\0x04'
CMD_ID_SEND = 'r'
begin_of_t = '&'
end_of_t = '@'
SEP_CHAR = '|'
MSG_INTERVAL = 5

print('Receive command is ' + str(CMD_ID_SEND))

disk_start: str = '/dev/'

class ParseException(Exception):
    msg: str
    
    def __init(self, err):
        self.str = err
        
    def getMessage():
        return msg
try:
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1, write_timeout=20,
                        parity=serial.PARITY_NONE, bytesize=serial.EIGHTBITS)  # open serial port
except:
    ser = serial.Serial('/dev/ttyACM1', 9600, timeout=1, write_timeout=20,
                        parity=serial.PARITY_NONE, bytesize=serial.EIGHTBITS)

def to_byte_array(f_val: float):
    return bytearray(struct.pack("f", f_val))
    #return binascii.hexlify(struct.pack('f',f_val))

def run_command(command: str):
    result = subprocess.run(command, stdout=subprocess.PIPE)
    return result.stdout.decode('utf-8')

def parse_mem_value(src: str, which_one: str):
    start_pos: int = src.find(which_one)
    
    if not start_pos >= 0:
        err = 'Error parsing "' + str + '" for memory variable [1]: ' +\
              which_one
        
        raise ParseException(err)
    
    end_pos = src.find(mem_end, start_pos)
    
    if not end_pos > 0:
         err = 'Error parsing "' + str + '" for memory variable [2]: ' +\
              which_one
         
         raise ParseException(err)
    
    start_pos += len(which_one)
    
    parse_val = src[start_pos:end_pos].strip()
    
    #print('Mem parsing results:')
    #print("\tSource string: " + src)
    #print("\tMemory value to search: " + which_one)
    #print('\tStart pos: ' + str(start_pos))
    #print('\tEnd pos: ' + str(end_pos))
    #print('\tMethod parse_mem_val for memory variable ' + which_one +\
    #      ' parsed the value: ' + parse_val) 
    
    mem_val = float(parse_val) / (1000 * 1000)
    
    #return to_byte_array(mem_val)
    return str(mem_val)

def parse_memory_output(output: str):
   m_total = parse_mem_value(output, mem_first)
   m_free = parse_mem_value(output, mem_second)
   m_avail = parse_mem_value(output, mem_third)
   
   return (m_total, m_free, m_avail)
   

def parse_disk_output(output: str):
    start_pos = output.find(disk_start)
    
    if not start_pos > 0:
        err = 'Error parsing disk output [1]: ' + output
        
        raise ParseException(err)

    
    # now find space after /dev/partition
    start_pos = output.find(' ', start_pos)
    
    if not start_pos > 0:
        err = 'Error parsing disk output [2]: ' + output
        
        raise ParseException(err)
    
    # now find first G
    end_pos = output.find('G', start_pos)
    
    if not end_pos > 0:
        err = 'Error parsing disk output [3]: ' + output
        
        raise ParseException(err)
    
    disk_total = float(output[start_pos:end_pos].strip())
    
    print('Disk total = ' + str(disk_total))
    
    # Now get second number
    start_pos = end_pos + 1
    
    # default to 0 free
    disk_used = disk_total
    
    end_pos = output.find('G', start_pos)
    
    if not end_pos > 0:
        end_pos = output.find('M', end_pos + 1)
        
        if not end_pos > 0:
            pass
        else:
            disk_used = float(output[start_pos:end_pos].strip())
    else:
        disk_used = float(output[start_pos:end_pos].strip())
    
    disk_free = disk_total - disk_used
    
    print("Disk free = " + str(disk_free))
    
    #return (to_byte_array(disk_total), to_byte_array(disk_free))
    return (str(disk_total), str(disk_free))

# command returns value like 51500
def get_temp():
    output = run_command(temp_command)
    
    output = output.strip();
    
    print('Output of get_temp.sh: ' + str(output))
    
    t_val = float(output) / 1000
    #return to_byte_array(t_val)
    return str(t_val)

# command returns 3 lines
#-- MemTotal:        7880552 kB
#-- MemFree:         2411524 kB
#-- MemAvailable:    4330752 kB
def get_memory():
    output = run_command(memory_command)
    return parse_memory_output(output)

# command returns 2 lines
#-- Filesystem      Size  Used Avail Use% Mounted on
#-- /dev/mmcblk2p2   57G   13G   42G  23% /

def get_disk_space():
    output = run_command(disk_command)
    return parse_disk_output(output)

def delay(millis: int):
    time.sleep(millis / 1000.0)
 

while True:
    parse_err = False
    
    print('Checking values');
    
    try:
        temp: str = get_temp()
    
        mem_total, mem_free, mem_available = get_memory()

        disk_free, disk_total = get_disk_space()
    except ParseException as ex:
        parse_err = True
        print("Error parsing values: " + ex.getMessage())

    if not parse_err:
        
        print('CMD_ID_SEND: ' + str(CMD_ID_SEND) + ', length=' + str(len(CMD_ID_SEND)))
        print('temp: ' + str(temp) + ', length=' + str(len(temp)))
        print('mem_total: ' + str(mem_total) + ', length=' + str(len(mem_total)))
        print('mem_free: ' + str(mem_free) + ', length=' + str(len(mem_free)))
        print('mem_available: ' + str(mem_available) + ', length=' + str(len(mem_available)))
        print('disk_free: ' + str(disk_free) + ', length=' + str(len(disk_free)))
        print('disk_total: ' + str(disk_total) + ', length=' + str(len(disk_total)))
        print('end_of_t: ' + str(end_of_t) + ', length=' + str(len(end_of_t)))
        
        msg_out = ""
        
        msg_out += begin_of_t
        msg_out += SEP_CHAR
        msg_out += CMD_ID_SEND;
        msg_out += SEP_CHAR
        msg_out += temp[0:4]# 4 characters
        msg_out += SEP_CHAR
        msg_out += mem_total[0:4]# 4 characters
        msg_out += SEP_CHAR
        msg_out += mem_free[0:4]# 4 characters
        msg_out += SEP_CHAR
        msg_out += mem_available[0:4]# 4 characters
        msg_out += SEP_CHAR
        msg_out += disk_free[0:4]# 4 characters
        msg_out += SEP_CHAR
        msg_out += disk_total[0:4]# 4 characters
        msg_out += SEP_CHAR
        msg_out += end_of_t
        
        msg_out = ascii(msg_out)
        bytes_out = bytearray(msg_out, 'ascii')
        
        print('Length of msg_out = ' + str(len(msg_out)))
        print('Length of bytes_out = ' + str(len(bytes_out)))
        
        print("Message out contents: " + str(msg_out))
        print('Bytes out contents: ' + str(bytes_out))
        
        try:
            ser.write(bytes_out)

            ser.flush()
            
            print("Data written")    
        except serial.serialutil.SerialTimeoutException:
            print('Timeout occured')
        except serial.serialutil.SerialException as ex:
            print('Serial exception: ' + str(ex))
    else:
        print("Error parsing data.")

    time.sleep(MSG_INTERVAL)


