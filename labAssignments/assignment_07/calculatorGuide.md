# UDP Scientific Calculator Guide

## 1. Compile

```bash
gcc server.c -o server -lm
gcc client.c -o client
```

## 2. Setup Mininet

```bash
sudo mn --topo single,2
xterm h1 h2
```

Launch Wireshark on **h1**:

```bash
h1 wireshark &
```

Filter:

```
udp.port == 8080
```

## 3. Execution

Server (h1):

```bash
./server
```

Client (h2):

```bash
./client 10.0.0.1
```

## 4. Commands

```
add x y
sub x y
mul x y
div x y

sin x
cos x
tan x
log x
sqrt x
inv x
```

## 5. Analysis

If the client shows a timeout but Wireshark captures a UDP packet without a reply, packet loss is confirmed.
