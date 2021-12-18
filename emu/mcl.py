#!/usr/bin/env python3

# https://github.com/YYYwWwhhh/TTouching-Android/tree/5f2c3b6e3e814e87dbe41c40387cadae938032af/hiflyingmartlink/src/main/java/com/hiflying/smartlink/v7


import socket
import select
import sys
import yaml

DEVICE_MAC = "00:00:00:00:00:00"

if len(sys.argv) > 1:
    MAC = sys.argv[1]
else:
    with open("secrets.yaml", "r") as stream:
        try:
            yml = yaml.safe_load(stream)
            mac = yml["device_mac"]
            if mac:
                MAC = mac
        except yaml.YAMLError as exc:
            pass
        except KeyError as exc:
            pass

print(f"Using MAC address: {MAC}")

# 239.48.0.0:6000

PORT_RECEIVE_SMART_CONFIG = 49999
PORT_SEND_SMART_LINK_FIND = 48899

ANY = "0.0.0.0"
MCAST_ADDR = "239.0.0.0"
# MCAST_ADDR = "192.168.1.255"
MCAST_PORT = PORT_SEND_SMART_LINK_FIND
# 239.48.0.0:6000

P_a_count = 0
P_5_count = 0
link_done = False
airkiss_sock = None

PORT_AIRKISS_SEND = 10000
PORT_AIRKISS_RECV = 47777

DATA_SMARTLINKFIND = b"smartlinkfind"
DATA_AA = b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
DATA_05 = b"\x05"
DATA_HFASSIST = b"HF-A11ASSISTHREAD"


airkiss_received = False

server_ip = None


def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    ip = s.getsockname()[0]
    s.close()
    return ip


def send_airkiss(ip):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        addr = (ip, PORT_AIRKISS_SEND)
        sock.connect(addr)
        sock.send(b"\x00")
        # print("<<< %s" % sock.recvfrom(8192))


def send_cmd(ip, cmd):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        addr = (ip, PORT_SEND_SMART_LINK_FIND)
        sock.connect(addr)
        sock.send(cmd.encode("utf-8"))
        # print("<<< send_cmd: %s" % sock.recvfrom(8192))


def do_read(sock):
    global P_a_count
    global P_5_count
    global link_done
    global airkiss_received
    global server_ip

    try:
        data, addr = sock.recvfrom(1024)
    except socket.error as e:
        pass
    else:
        if data == DATA_SMARTLINKFIND:
            print(data, sock.getsockname(), addr)
            if False and not airkiss_received:
                send_airkiss(addr[0])
            else:
                msg = "smart_config " + DEVICE_MAC  # .replace(":", "").lower()
                sent = sock.sendto(msg.encode("utf-8"), addr)

        elif data == DATA_HFASSIST:
            print(data, sock.getsockname(), addr)
            # airkiss_received = True
            msg = get_ip() + "," + DEVICE_MAC
            sent = sock.sendto(msg.encode("utf-8"), addr)

        elif data == DATA_AA:
            print("<<< Received aa , len = %d" % len(data))
            msg = data
            sent = sock.sendto(data, addr)
            P_a_count = P_a_count + 1
            # print(
            #     "AA sent %s bytes back to %s, p_a_count = %d" % (sent, addr, P_a_count)
            # )

        elif data[0] == 5:
            print("<<< Received 05 , len = %d" % len(data))
            msg = data
            sent = sock.sendto(msg, addr)
            P_5_count = P_5_count + 1
            # print(
            #     "05 sent %s bytes back to %s, P_5_count = %d" % (sent, addr, P_5_count)
            # )
            msg = "smart_config " + DEVICE_MAC  # .replace(":", "").lower()
            sent = sock.sendto(msg.encode("utf-8"), addr)

        else:
            print("From: %s, data: %s" % (addr, data))


# 46088
def main2():
    global airkiss_sock

    sockets = []
    for port in [
        PORT_SEND_SMART_LINK_FIND,
        # PORT_AIRKISS_RECV,
        # 6000,
    ]:  # , 10000, 6000, 49999
        sock = None
        # Create a UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # if port == 10000:
        #     airkiss_sock = sock

        # Allow multiple sockets to use the same PORT number
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        # Bind to the port that we know will receive multicast data
        sock.bind(("0.0.0.0", port))

        print("socket on %d" % port)
        sock.setsockopt(
            socket.IPPROTO_IP,
            socket.IP_ADD_MEMBERSHIP,
            socket.inet_aton("239.48.0.0") + socket.inet_aton(ANY),
        )
        if port == 6000:  # MulticastSmartLinkerSendAction
            sock.setsockopt(
                socket.IPPROTO_IP,
                socket.IP_ADD_MEMBERSHIP,
                socket.inet_aton("239.48.0.0") + socket.inet_aton(ANY),
            )
        # if port == PORT_SEND_SMART_LINK_FIND:
        #     status = sock.setsockopt(
        #         socket.IPPROTO_IP,
        #         socket.IP_ADD_MEMBERSHIP,
        #         socket.inet_aton("239.48.0.0") + socket.inet_aton(ANY),
        #     )

        sockets.append(sock)

    # sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)  # UDP
    # sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    # sock.bind(("0.0.0.0", 0))
    # for x in range(10):
    #     sock.sendto(b"\x00", ("255.255.255.255", PORT_AIRKISS_SEND))
    # sock.close()

    while not server_ip:
        readable, writable, exceptional = select.select(sockets, [], [])
        for sock in readable:
            do_read(sock)
    for sock in sockets:
        sock.close()


main2()
