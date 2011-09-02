import socket
import time
import sys

host = sys.argv[1]
port = sys.argv[2]
count = int(sys.argv[3])

start = time.time()
counter = 0
while counter < count:
    counter += 1
    sock = socket.create_connection((host, int(port)))
    sock.recv(256)
    sock.close()

end = time.time()
print "connection rate = " + str(count / (end - start)) + " connections per second"
