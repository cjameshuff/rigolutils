
Each connection is to one specific device. The server spawns a subprocess for each connection, allowing simultaneous access to multiple devices (including such things as simultaneously downloading waveforms from multiple devices) as hardware permits. However, only one connection is permitted to each device.

Once a connection to the server is established, a ConnectToDevice command must be issued. DeviceAccess commands can then be issued to perform reads and writes to the device.

TODO:

* spawn the subprocess after the ConnectToDevice command rather than immediately after connection, allowing the server to track device connections?
* implement other commands
* allow server to be configured to broadcast the result of a given query periodically, to allow multiple users without overloading the device?


## Packet format:

All values in network byte order
Command: 8 byte header followed by variable amount of data

	uint cmd:16
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32
	
	uint payload:payloadSize bytes

Framing is done SLIP-style:
0xFF is an escape character
0xFF 0xFE escapes the 0xFF character
0xFF 0xFD terminates the current frame


### Ping (cmd = 0x0000)

Variable length:

	uint cmd:16 = 0x0000
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32
	
	uint payload:payloadSize bytes

Measure lag. Server responds immediately with identical packet. Payload can be a timestamp, padding, or absent, it is simply returned to the sender.


### Disconnect (cmd = 0x0002)


### ConnectToDevice (cmd = 0x0200)

Variable length:

	uint cmd:16 = 0x0200
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32 >= 4
	
	uint vendorID_productID:32 (vendor ID in high 2 bytes, product ID in low 2 bytes)
	uint sernum:payloadSize bytes

Response has same format. If connection is successful, response has serial number of connected device. If connection fails, response has no payload.

Connect to device with given vendor and product ID and serial number. Connect to first found if no serial number provided.


### DeviceWrite (cmd = 10, cmd2 = 0)

Command packet:

	uint cmd:16 = 0x0F00
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32 >= 4
	
	uint readSize:32
	writeData: payloadSize - 4 bytes

Response packet (only sent if read requested):

	uint cmd:16 = 0x0F00
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32
	
	readData: payloadSize bytes


## Service Discovery

The server supports discovery by responding to a PING message broadcast on UDP. The server responds with the server name, address, and a list of devices.
The broadcast address and port are 225.0.0.50:49393. (TODO: IPv6 discovery)

The UDP messages are of identical format to the TCP messages, except cmd is always 0x0000 and the sequence number pair is always 0x55:0xAA.

Discovery query:
	uint cmd:16 = 0x0000
	uint seqnum:8 = 0x55
	uint seqnum2:8 = 0xAA
	uint payloadSize:32
	
	uint numSupportedDevices:32
	uint supportedDevices: numSupportedDevices*4 bytes
	

The payload consists of a list of supported VID/PID pairs (as used in the ConnectToDevice message).

The response has an identical header, with a payload of the server name and a list of entries for the matching connected devices.
Server name:
	uint serverNameSize:32
	uint serverName: serverNameSize bytes

Followed by 0 or more of:
	uint VIDPID:32
	uint sernumSize:32
	uint sernum: sernumSize bytes
	




