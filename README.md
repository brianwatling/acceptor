#Acceptor - TCP Listener and Connection Distributor

This daemon will listen for connections on a TCP port and distribute them to other processes. This can be used to allow non-root processes to bind privileged ports. Acceptor enables servers to scale using processes while still binding a single port (and no forking required).

#Example

-Start Acceptor. Specify the port it should listen on. Specify a name for the Unix socket used to send TCP connections to other daemons.
    ./acceptor --port=10000 --name=acceptorTest

-Start a daemon process which receives connections from acceptor, for example the included 'tester' app.
    ./tester --name=acceptorTest --message=SomeTestMessage

-Now acceptor will pass new connections to 'tester', which will send the string "SomeTestMessage" to the client.
-Start a telnet session:
    telnet localhost 10000
    SomeTestMessage

-Add another 'tester' app to have acceptor split connections between both processes:
    ./tester --name=acceptorTest --message=SomeOtherMessage

-Start telnet sessions, watch the different messages:
    telnet localhost 10000
    SomeTestMessage
    telnet localhost 10000
    SomeOtherMessage

#Usage

    --help          produce help message
    --name or -n    name of the UNIX socket to bind
    --host or -h    default: localhost. host to listen for tcp connections on
    --port or -p    port to listen for tcp connections on
    --listenBacklog or -l default: 5. backlog parameter passed to listen()

#TODO

-better load balancing (currently only supports round-robin)
-possibly allow connection receivers to send messages to acceptor (keep alive, indicate how loaded they are)
-mechanism for client connections to specify which backend they'd like to be passed to? routing?

#License

Public domain
