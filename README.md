# W5500-ESPAsyncWebServer-V2
W5500 Ethernet Adapter based version of ESPAsyncWebServer Supporting Websocket, Version 2

Version 2 has significant improvements over version 1: 
 - Truly Asynchronous sending only updated infrormation to all connected clients with no webpage refreshing required.
 - Allows user configureable pin output state, pin "On" can be set to High or Low, "Off" can be set to High or Low. Useful if utilizing the output pin to drive a relay where the on state = pin low.
 - Adds an additional mode "Auto" that can toggle the output on or off on other conditions such as a temperature value. Includes an example "Simulated Temperature" that can be increased with - or + Serial Monitor message and code that will turn on when simulated temp is set above 25 if "Auto" mode is selected.
 - Webpage correctly reports state and mode and has a drop down menu used to change the state.
