
import socket
import re


class vatt_filters:

    def __init__(self):
        pass

    def connect(self, ip:str="10.0.1.108", port:int=7623):
        soc = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
        HOST = socket.gethostbyname(ip)
        soc.settimeout(0.1)
        soc.connect( ( HOST, int( port ) ) )
        return soc


    def getfilters(self):
        conn = self.connect()
        qrystring = b"<getProperties version='1.7' device='FILTERS' />"
        conn.send(qrystring)
        regex = re.compile("message=\"(\w*):(\w*) (\w*):(\w*)\"")
        resp = ''
        while True:
            try:
                resp+= conn.recv(100).decode()
            except socket.timeout:
                if ">" in resp[-4:]:
                    break
                else:
                    raise socket.timeout(
                        "Could not get a response from {} with string {}"
                        .format(str(conn), qrystring.encode()))

            if ">" in resp[-4:]:
                break


        match = regex.search(resp)
        if match is not None:
            tmatch = match.groups()
            if len(tmatch) != 4:
                raise ValueError( "Could not get filters from {}".format(resp))
            fdict = {tmatch[0]:tmatch[1], tmatch[2]:tmatch[3]}

        else:

            raise ValueError( "Could not get filters from {}".format(resp))

        return fdict








