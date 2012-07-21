rigolwfm
========

C++ code and a simple tool for reading .wfm waveform files from Rigol DS1102E digital oscilloscopes

Data is read and converted to floating point (scaled in volts) for further operations. The main other parameter of interest for interpreting the data is fs in RigolWaveform, the sample frequency in Hz. Numerous parameters are extracted, but most can be ignored (as they are only relevant for rescaling the original integer data or replicating the oscilloscope display configuration).


Known issues with the reader:
=============================

* The first 4 or so data points appear to be garbage.
* Not all parameters are read. A notable omission is most of the trigger settings.

