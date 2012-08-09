Rigol DS1000 utilities
=====

Utilities and code for working with Rigol DS1000 series digital oscilloscopes (such as the DS1102E), including code for reading .wfm waveform files and for accessing them via USB.

Oscilloscope access is implemented via a generic server which handles direct communication with the oscilloscope or oscilloscopes, and accepts commands and sends responses via a TCP connection. This allows client programs to easily access instruments across a network (over a wireless connection, for instance), and allows them to be implemented in any language or platform that supports such network access. An abstraction layer allows direct USB and remote networked device connections to be swapped out without affecting higher level code dealing with oscilloscope functionality.

Components
=====

* scopeserv: Server, handles direct communication to oscilloscopes
* scopecmd: send commands to oscilloscopes via the command line, download waveform data
	* Examples:
		* `scopecmd idn` - get identity string for first scope found on USB
		* `scopecmd -u 192.168.0.21 get-scope-params` - Get a dump of all scope parameters from a remote scope
		* `scopecmd -s DS1EB104702974 rst` - Reset parameters on a specific scope attached via USB.
* scopev: plot waveform data, generate an image file or display on screen



Suggestions
=====

* Collect and merge data from multiple oscilloscopes
* Put the server on a BeagleBoard, Gumstix, plug computer, etc to add network connectivity to your scope, for:
	* data logging
	* physical and electrical isolation from the scope and system being measured, or of multiple scopes from each other
		* use a wireless connection for even more isolation


Status
=====

Server:

* Basic oscilloscope access works. The server can accept connections, forward commands, and send responses.
* Needs work to improve robustness, error handling, etc

Client:

* Can connect to a server and interact with remote devices
* Needs work to improve robustness, error handling, etc

Utilities:

* scopecmd allows basic command-line access to oscilloscopes
* Code exists for pulling data out of the oscilloscope and generating simple plots. Code needs to be cleaned up and turned into some usable tools:
	* waveform plotting
	* data acquisition
	* interactive scope control and real-time viewing
* Code exists for reading most data from a .wfm waveform file. Again, need to make tools using it.


Other:

* Protocol needs cleaned up, useful commands implemented and useless ones culled.
* Documentation.


TODO
=====

* Config files, scripting
* Server, device discovery: broadcast UDP query for servers or for a specific device by serial number
* device listing: query server for all devices
