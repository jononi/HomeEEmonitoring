# HomeEEmonitoring
Residential electric energy monitoring and logging based on OpenEnergyMonitor project and STM's DISCO-L475VG-IOT01A board

## Current status
* Use of separate threads for analog sensing task and communication tasks.
* Tested hardware timer (TIM3) interrupt to toggle an LED alongside regular mbed-os task switching. (software timer interrupts can interfere with this interrupt, to be checked later) 
* added a simple Nextion UI class with basic icon on/off functions for now. This is will be running in its own thread later on.
* Next upgrade: add timer triggered analog readings + saving analog reads in a file in the on-board qspi flash memory.
