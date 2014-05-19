#!/usr/bin/env python

# This script presents the fvcontroller RS485 interface as a
# socket-based service.  Only one connection is dealt with at once;
# others are queued.

# The script ensures that no controller is selected between
# connections, and that controllers are in a known state (empty rxbuf,
# nobody transmitting) waiting for commands.  It provides an explicit
# TIMEOUT response if a controller does not respond, and deals with
# returning the RS485 network to the default state if a timeout
# occurs.

import serial
import SocketServer
import time

def full_reset(s):
    """Return the bus to a known state: terminate any previous command
    with \n, send a SELECT NONE\n, and discard everything from the
    receive buffer.

    """
    # Terminate any partly-sent command.  We have to wait after
    # sending this for up to 0.1s for any output from the currently
    # selected controller to be completed; we receive and discard this
    # output.  No controller will send more than one line.
    s.write("\n")
    old_timeout=s.timeout
    s.timeout=0.1
    s.readline()
    s.timeout=old_timeout
    s.write("SELECT NONE\n")
    
class ConnectionHandler(SocketServer.StreamRequestHandler):
    def handle(self):
#        print("Connection opened.")
        for data in self.rfile:
            data=data.strip()
#            print(data)
            s.write("%s\n"%data)
            response=s.readline()
            if response=="": response="TIMEOUT\n"
            if response[-1]!="\n": response="CORRUPT\n"
            try:
                self.wfile.write(response)
            except:
                break
#            print(response)
#        s.write("SELECT NONE\n")
#        s.flush()
        # Wait for this command to complete before we drop the connection.
        # 12 bytes at 960 bytes per second is about 0.013s
#        time.sleep(0.013)
#        print("Connection closed.")
    
if __name__=="__main__":
    HOST,PORT="localhost",1576

    server=SocketServer.TCPServer((HOST,PORT),ConnectionHandler)

    s=serial.Serial("/dev/fvcontrollers",timeout=0.5)
    full_reset(s)

    server.serve_forever()
