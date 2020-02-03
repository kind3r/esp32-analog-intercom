# Logical flow

## Boot
- set pin modes
- restore WiFi connection parameters (IP, netmask, gateway, DNS) from EEPROM if they exist and connect
- if no WiFi paramters exist, use regular WiFi connect with DHCP and then save the connection parameters to EEPROM
- set time
- set sensor state in Home Assistant (or other automation software) as ringing so it can send notifications etc.
- setup webserver for incomming commands
- setup MDNS

## Loop
- monitor incoming http calls from Home Assistant for OPEN command
- monitor OPEN and TALK gpio pins
- set sensor state in Home Assistant as idle as we don't know when the intercom will stop ringing and power will be out

## OPEN command
In order to be able to open the door the intercom need to have at least one talk session of 2(?) seconds.

- set TALK pin to HIGH for 2 seconds
- set TALK pin to LOW
- set OPEN pin to HIGH (door should now open)

