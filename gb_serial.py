
import serial
from scottSock import scottSock
import time


class gbtalk():
    def __init__(self, port='/dev/ttyUSB0'):
        self.conn = serial.Serial(port, 115200, timeout=2.0)

    def write(self, msg):
        output = msg+" "

        self.conn.write(output.encode())

    def read(self):

        resp = ''
        while 1:
            c = self.conn.read().decode()
            if c in ['\r', '\n', '']:
                break
            else:
                resp += c

        return resp


    def getTelem( self, loop_count=20 ):
        for a in range(loop_count):
            self.write("GOSUB(998)")
            resp = None
            while resp != '':
                resp = self.read()
                print(resp)


def highbits(val):
    for a in range(16):
        print(a, (val & (1<<a)))


def read(conn=None):

    conn = scottSock("10.0.3.86", 10001, timeout=0.2)
    resp = conn.listen()
    conn.close()
    return resp


def write(msg, conn=None):
    conn = scottSock("10.0.3.86", 10001)
    conn.talk(msg+" ")
    conn.close()


def serialfix():

    conn = scottSock("10.0.3.86", 10001)
    conn.talk("\x80")
    conn.talk(" ")
    conn.close()


def getTelem():
    serialfix()
    time.sleep(0.5)
    write("GOSUB(998)")
    time.sleep(0.1)
    val = read()
    print(val.replace('\r', '\n'))
