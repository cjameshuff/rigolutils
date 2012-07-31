
Currently, only simple device connection and access is implemented

Each connection is to one specific device. The server spawns a subprocess for each connection, allowing simultaneous access to multiple devices (including such things as simultaneously downloading waveforms from multiple devices) as hardware permits. However, only one connection is permitted to each device.

Once a connection to the server is established, a ConnectToDevice command must be issued. DeviceAccess commands can then be issued to perform reads and writes to the device. The connection will be dropped if no commands are received for a given amount of time.

TODO:

* spawn the subprocess after the ConnectToDevice command rather than immediately after connection, allowing the server to track device connections?
* implement other commands
* allow server to be configured to broadcast the result of a given query periodically, to allow multiple users without overloading the device?


## Packet format:

All values in network byte order
Command: 12 byte header followed by variable amount of data

	uint cmd:8
	uint cmd2:8
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32
	uint unused:32
	uint payload:payloadSize

Ack: 4 byte packet

	uint cmd:8
	uint cmd2:8
	uint seqnum:8
	uint seqnum2:8


### Ping (cmd = 0, cmd2 = 0)

Measure lag and function as a keep-alive function.
If KeepAlive is not set to 0, connections will be dropped after KeepAlive seconds of inactivity.


### SetKeepAlive (cmd = 0, cmd2 = 1)

### Disconnect (cmd = 0, cmd2 = 2)

### ListDevices (cmd = 1, cmd2 = 0)

	uint cmd:8 = 2
	uint cmd2:8 = 1
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32
	uint vendorID:16
	uint productID:16

List serial numbers of all devices matching vendor ID and product ID.


### ConnectToDevice (cmd = 1, cmd2 = 1)

Variable length:

	uint cmd:8 = 2
	uint cmd2:8 = 1
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32
	uint vendorID:16
	uint productID:16
	uint sernum:payloadSize

Response is standard ack.

Sequentially tests each device with given vendor and product IDs, sending "*IDN?" to each.
Connects to the first device that responds with the desired serial number.

If given device ID is 255, an ID from the range 0-254 is assigned and used in the ack.


### ResetDevice (cmd = 5)

16 bytes:

	uint cmd:8 = 5
	uint cmd2:8 = device ID, starting at 0x00
	uint seqnum:8
	uint seqnum2:8
	uint payloadSize:32 = 0
	uint unused:32
	uint payload:payloadSize

Response is standard ack.


## DeviceAccess (cmd = 10, cmd2 = 0)

Command packet:

	cmd: = 0x01
	cmd2: unused
	payloadSize: writeSize, command size
	other: readSize, number of bytes to read after sending command
	payload: writeSize bytes: command string

Response packet:

	cmd: = 0x01
	cmd2: device ID, starting at 0x00
	payloadSize: bytesRead, number of bytes actually read back from device
	other: not used
	payload: writeSize bytes: response string
