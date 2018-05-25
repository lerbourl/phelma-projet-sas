#!/usr/bin/env python3

import smbus
import RPi.GPIO as GPIO
import json
import sys
import time
import os
from subprocess import call
from SimpleWebSocketServer import SimpleWebSocketServer, WebSocket

# This class inherits from WebSocket.
# It receives messages from the Scratch and reports back for any digital input
# changes.
class SAS_SERVER(WebSocket):
    
    def handleMessage(self):
        # get command from Scratch2
        payload = json.loads(self.data)
        print(payload)
        client_cmd = payload['command']
        
        if client_cmd == 'open_close_door':
            client_order = payload['order']
            if client_order == 'ouvrir':
                bus.write_byte(address, 0xa1)
            if client_order == 'fermer':
                bus.write_byte(address, 0xa2)
                
        elif client_cmd == 'measure':
            sensor = payload['sensor']
            pin = 0
            if sensor == 'etat bouton rouge':
                pin = 11
            elif sensor == 'etat plaque de pression':
                pin = 7
            elif sensor == 'distance detecteur':
                bus.write_byte(address, 0xa3)
                result = 0xb0
                while (result != 0xb3):
                    time.sleep(0.05)
                    result = bus.read_byte(address)
                result = bus.read_byte(address)
            else: print("erreur capteurs")
            if (pin != 0):
                if GPIO.input(pin):
                    result = 0
                else:
                    result = 1
            payload = {'report': 'measure', 'result': result}
            print('callback', payload)
            msg = json.dumps(payload)
            self.sendMessage(msg)
            
        elif client_cmd == 'display':
            text = payload['txt']
            floor = payload['floor']
            print(text, floor)
            data = [ord(c) for c in text+floor]
            bus.write_block_data(address, 0xa4, data)
            
        elif client_cmd == 'ask_code':
            bus.write_byte(address, 0xa6)
            code = 0xb0
            intcode = 0
            while (code == 0xb0):
                time.sleep(0.05)
                code = bus.read_byte(address)
            if (code == 0xb1):
                for i in range(4):
                    intcode += 10**(3-i) * int(bus.read_byte(address))
            if (code == 0xb2):
                intcode = 66666
            payload = {'report': 'code', 'number': intcode}
            print('callback', payload)
            msg = json.dumps(payload)
            self.sendMessage(msg)
            
        elif client_cmd == 'clear_lcd':
            bus.write_byte(address, 0xa7)
            
        elif client_cmd == 'display_digicode_instructions':
            bus.write_byte(address, 0xa5)
            
        elif client_cmd == 'ready':
            pass
        
        else:
            print("Unknown command received", client_cmd)

    def handleConnected(self):
        print(self.address, 'connected')

    def handleClose(self):
        payload = {'report': 'close'}
        print('callback', payload)
        msg = json.dumps(payload)
        self.sendMessage(msg)
        print(self.address, 'closed')

def run_server():
    # checking running processes.
    # if the backplane is already running, just note that and move on.
    global bus, address
    bus = smbus.SMBus(1)
    address = 0x12 #arbitraire
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(11, GPIO.IN) #bouton rouge
    GPIO.setup(7, GPIO.IN) #plaque de pression
    os.system('scratch2&')
    server = SimpleWebSocketServer('', 9000, SAS_SERVER)
    server.serveforever()

if __name__ == "__main__":
    try:
        run_server()
    except KeyboardInterrupt:
        sys.exit(0)
