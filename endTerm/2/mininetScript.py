#!/usr/bin/python

from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSController
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink
from mininet.topo import Topo
import os
import time

class SimpleTopo(Topo):
    "Two hosts connected by a 10Mbps link."
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        self.addLink(h1, h2, bw=10)

def compile_code():
    info( 'Compiling C code...\n' )
    os.system('gcc server.c -o server -lpthread')
    os.system('gcc client.c -o client -lpthread')

def run_experiment():
    compile_code()

    topo = SimpleTopo()
    # TCLink for bandwidth limitation
    # controller=None because host are directly connected aka no switches.
    net = Mininet(topo=topo, link=TCLink, controller=None)
    
    net.start()
    
    h1 = net.get('h1')
    h2 = net.get('h2')

    info( 'Starting packet capture on h1...\n' )
    # Capture traffic on h1's interface to a file named 'traffic.pcap'
    h1.cmd('tcpdump -w traffic.pcap -i h1-eth0 &')

    time.sleep(2)

    info( 'Starting Server on h1...\n' )
    h1.cmd('./server > server_log.txt &')
    time.sleep(1)

    info( 'Starting Client on h2 (8 concurrent streams)...\n' )
    h2.cmd('./client')
    time.sleep(15)

    info( 'Stopping experiment...\n' )
    h1.cmd('killall tcpdump')
    h1.cmd('killall server')
    net.stop()

    info( 'Experiment Finished.\n' )
    info( 'Output files: traffic.pcap, server_log.txt\n' )

if __name__ == '__main__':
    setLogLevel('info')
    run_experiment()
