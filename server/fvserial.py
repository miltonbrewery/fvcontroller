#!/usr/bin/env python3

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
import socketserver

def full_reset(s):
    """Return the bus to a known state

    Terminate any previous command with \n, send a SELECT NONE\n, and
    discard everything from the receive buffer.
    """
    # Terminate any partly-sent command.  We have to wait after
    # sending this for up to 0.1s for any output from the currently
    # selected controller to be completed; we receive and discard this
    # output.  No controller will send more than one line.
    s.write(b"\n")
    old_timeout = s.timeout
    s.timeout = 0.1
    # Read until we time out
    foo = True
    while foo:
        foo = s.read()
    s.timeout = old_timeout

class ConnectionHandler(socketserver.StreamRequestHandler):
    def handle(self):
        for data in self.rfile:
            data = data.strip()
            s.write(data + b"\n")
            response = s.read_until()
            # A floating line produces \0 characters.  Remove them.
            response = response.replace(b'\0', b'')
            if response == b"":
                response = b"TIMEOUT\n"
            elif response[-1] != ord("\n"):
                response = b"CORRUPT\n"
            try:
                self.wfile.write(response)
            except:
                break

class ReuseTCPServer(socketserver.TCPServer):
    allow_reuse_address = True

if __name__=="__main__":
    HOST, PORT = "localhost", 1576

    server = ReuseTCPServer((HOST, PORT), ConnectionHandler)

    s = serial.Serial("/dev/ttyUSB0", timeout=1.0)
    full_reset(s)

    server.serve_forever()
