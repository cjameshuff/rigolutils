Rigol DS1000 utilities
=====

Utilities and code for working with Rigol DS1000 series digital oscilloscopes (such as the DS1102E), including code for reading .wfm waveform files and for accessing them via USB.

Oscilloscope access is implemented via a generic server which handles direct communication with the oscilloscope or oscilloscopes, and accepts commands and sends responses via a TCP connection. This allows client programs to easily access instruments across a network (over a wireless connection, for instance), and allows them to be implemented in any language or platform that supports such network access. An abstraction layer allows direct USB and remote networked device connections to be swapped out without affecting higher level code dealing with oscilloscope functionality.


Status
=====

Server:

* Basic oscilloscope access works. The server can accept connections, forward commands, and send responses.
* Needs work to improve robustness, error handling, etc

Client:

* Can connect to a server and interact with remote devices
* Needs work to improve robustness, error handling, etc

Utilities:

* Code exists for pulling data out of the oscilloscope and generating simple plots. Code needs to be cleaned up and turned into some usable tools for plotting, dumping data, real-time viewing, etc.
* Code exists for reading most data from a .wfm waveform file. Again, need to make tools using it.
* Oscilloscope commands need to be filled out.

Other:

* Protocol needs cleaned up, useful commands implemented and useless ones culled.
* Documentation.
