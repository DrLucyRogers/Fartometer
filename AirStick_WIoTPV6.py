# Publish MQTT data from the Rehau VOC air quality data to 
# Watson IoT Platform

# Bill Hymas
# Andy Stanford-Clark
# Sep'16
# Copyright (c) 2016 IBM Corporation

#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#  http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
 


import time
import usb.core
import sys
# import Watson IoT library
import ibmiotf.device

# you can change these
device = "VOCmeter"
event = "reading" 

# we need a UID as a command line parameter
if len(sys.argv) != 2:
    print("usage: %s unique_identifier" % sys.argv[0]);
    sys.exit(0)
else:
    device_id = sys.argv[1]
#end



# set up device credentials for IoTP
try:
  options = {
    "org": "quickstart",
    "type": device,
    "id": device_id,
    "auth-method": "",
    "auth-token": ""
  }
  client = ibmiotf.device.Client(options)
except ibmiotf.ConnectionException  as e:
    raise "problem defining credentials"
#end


def myCommandCallback(cmd):
    print("Data : %s" % cmd.data)
    print("Format : %s" % cmd.format)
    print("timestamp : %s" % cmd.timestamp)

    dir = cmd.data['cmd']
#end


# This is the control command for the stick.
# You write this command to the stick, and do two reads after that.
# This first read will return the voc value.
# The second will be something bogus that can be tossed.
# It can happen that a program using the stick ends with read data still pending
# This will cause problems the next time it is used.
# So at the beginning of a new program execution, the stick will be cleaned and reset.
command = "\x40\x68\x2a\x54\x52\x0a\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"

# find our airDevice
airDev = usb.core.find(idVendor=0x03EB, idProduct=0x2013)

# Bail if it is not found
if airDev is None:
    raise ValueError('airDevice not found')
else:
    #print "First we will try to get the device into a clean and reset state."
    #print "The device, but the kernel probably has it attached as a mouse."    
    print "detaching kernel driver"
    try:
        airDev.detach_kernel_driver(0)
    except Exception as e:
        print e
    print "claiming the device"
    usb.util.claim_interface(airDev, 0)
    print "flushing existing data . . ."
    try:
        result = airDev.read(0x81, len(command), 5000)
        print("got pending data that will be discarded")    
    except Exception as e:
        print ("flush read timed out")
        
    print "resetting device"
    airDev.reset()
    #print "After the reset, we need to start over"
    print "finding device again"
    airDev = usb.core.find(idVendor=0x03EB, idProduct=0x2013)
    while airDev == None:
        time.sleep(1)
        print "finding"
        airDev = usb.core.find(idVendor=0x03EB, idProduct=0x2013)
    print "detaching kernel driver"    
    try:
        airDev.detach_kernel_driver(0)
    except Exception as e:
        print e    
    print "claiming airDevice"
    usb.util.claim_interface(airDev, 0)        
           

# display useful information
print ("\napplication data topic: iot-2/type/%s/id/%s/evt/%s/fmt/json" %
       (device, device_id, event))
print ("broker: quickstart.messaging.internetofthings.ibmcloud.com")


# connect to IoT Platform
print ("Connecting to Watson IoT Platform...")
client.connect()
client.commandCallback = myCommandCallback
print ("... connected")

topic = "iot-2/evt/%s/fmt/json" % event
#print ("publishing on topic: %s" % topic)


while True:
    # Write the control command to the stick
    #result = airDev.write(0x02, command, 0, 5000 )
    result = airDev.write(0x02, command, 5000 )
    # Do the first read, which will return the voc
    result = airDev.read(0x81, len(command), 5000)
    # Fish and shift out the result
    voc = (result[3] << 8) + result[2]
    #print("voc " + str(voc) + " len " + str(len(result)))
    print("voc " + str(voc) )

    reading = voc

    client.publishEvent(event, "json", reading)

    result = airDev.read(0x81, len(command), 5000)
    # Do the second read, which just returns a 0 length result
    #print("read 2 len " + str(len(result)))    
    time.sleep(5)
                
                
