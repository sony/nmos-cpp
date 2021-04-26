20/4/2021 D Butler, BBC R&D

## NMOS C++ Implementation for the 5G-Records Project
--------------------------------------------------
Work in progress

## Introduction
------------
The 5G-Records nmos-cpp software is modified version of the open source Sony nmos-cpp software available on the Sony git repository ( https://github.com/sony/nmos-cpp ) and is subject to the open source license agreement described in the repository. 

The original nmos-cpp software creates a nmos-node and nmos-registry software. The nmos-node runs on a local media device and registers the device with the nmos-registry software running on separate server. Register devices can then be controlled by the nmos-js Ctrl App ( https://github.com/sony/nmos-js ).

The original nmos-node software employs IP addresses from the local media device for AMWA IS-04 NMOS Discovery and Registration Specification and AMWA IS-05 NMOS Device Connection Management Specification. The implementation creates identities and registers multicast audio, video, data and websocket streams employing hard coded parameters. The number of streams is set by a “how_many” json field in a config.json file.

The 5G-Records nmos-node software has been modified to allow registration and connection management of a remote media device. Relative to the original nmos-node software, the following changes have been made:
1.	A programmable source remote IP Address, MAC Address, Interface name and label.
2.	Creation of a programmable number of senders.
    -	Each sender has programmable label, unicast/multicast mode, payload type, format, destination IP Address and destination port number.
3.	Creation of a programmable number of receivers.
    -	Each receiver has programmable label, unicast/multicast mode, payload type and format type.


## Development Environment and Compiling
-------------------------------------
Original make and dependency info:

https://github.com/sony/nmos-cpp/blob/master/Development/CMakeLists.txt

https://github.com/sony/nmos-cpp/blob/master/Documents/Dependencies.md

The software can be compiled on Windows or Linux, the following are instructions for compiling on an Ubuntu 20.04 LTS virtual machine.


user@vm2:~$ sudo apt-get install cmake build-essential

user@vm2:~$ cmake --version <br />
    cmake version 3.16.3

user@vm2:~$ make --version <br />
    GNU Make 4.2.1

user@vm2:~$ sudo apt install python3-pip

user@vm2:~$ pip3 install conan

user@vm2:~$ sudo cp -r .local/bin/* /usr/local/bin/

user@vm2:~$ ls /usr/local/bin/ <br />
bottle.py  conan_build_info  cvlc      nvlc         qvlc       ristreceiver  rvlc  tqdm  vlc-wrapper
conan      conan_server      firebase  __pycache__  rist2rist  ristsender    svlc  vlc

user@vm2:~$ conan --version <br />
Conan version 1.33.0

For registration, a MDNS responder must be installed and configured

user@vm2:~$ wget https://opensource.apple.com/tarballs/mDNSResponder/mDNSResponder-878.270.2.tar.gz

user@vm2:~$ tar -xzvf mDNSResponder-878.270.2.tar.gz

user@vm2:~/mDNSResponder-878.270.2/mDNSPosix$ make os=linux

user@vm2:~/mDNSResponder-878.270.2/mDNSPosix$ sudo make os=linux install

Check the nsswitch.conf file. <br />
If 'mdns' does not already appear on the "hosts:" line, then add it right before 'dns' <br />

cp -f /etc/nsswitch.conf /etc/nsswitch.conf.pre-mdns <br />
sed -e '/mdns/!s/^\(hosts:.*\)dns\(.*\)/\1mdns dns\2/' /etc/nsswitch.conf.pre-mdns > /etc/nsswitch.conf <br />
cp nss_mdns.conf.5 /usr/share/man/man5/nss_mdns.conf.5 <br />
chmod 444 /usr/share/man/man5/nss_mdns.conf.5 <br />
cp libnss_mdns.8 /usr/share/man/man8/libnss_mdns.8 <br />
chmod 444 /usr/share/man/man8/libnss_mdns.8 <br />
/lib/libnss_mdns.so.2 /etc/nss_mdns.conf /usr/share/man/man5/nss_mdns.conf.5 /usr/share/man/man8/libnss_mdns.8 installed 

Add mdns4_minimal in .conf. if not present.

user@vm2:~/mDNSResponder-878.270.2/mDNSPosix$ pico /etc/nsswitch.conf
 /etc/nsswitch.conf <br />
 Example configuration of GNU Name Service Switch functionality. <br />
 If you have the `glibc-doc-reference' and `info' packages installed, try: <br />
 `info libc "Name Service Switch"' for information about this file. <br />
passwd:         files systemd <br />
group:          files systemd <br />
shadow:         files <br />
gshadow:        files <br />
hosts:          files mdns4_minimal [NOTFOUND=return] dns <br />
networks:       files <br />
protocols:      db files <br />
services:       db files <br />
ethers:         db files <br />
rpc:            db files <br />
netgroup:       nis <br />


user@vm2:~$ sudo apt-get install git <br />
[Note: users will need git.ebu.io permissions, SSO and SSH-RSA credentials to be configured]

user@vm2:~$ git clone git@git.ebu.io:david.butler/nmos-js.git <br />
user@vm2:~/nmos-cpp/Development$ mkdir build <br />
user@vm2:~$ cd nmos-cpp/Development/build/ <br />
user@vm2:~/nmos-cpp/Development/build$ cmake ..   -DCMAKE_BUILD_TYPE:STRING="Debug"   -DWERROR:BOOL="0"   -DBUILD_SAMPLES:BOOL="0"   -DBUILD_TESTS:BOOL="0"

user@vm2:~/nmos-cpp/Development/build$ make <br />
user@vm2:~/nmos-cpp/Development/build$ sudo make install <br />


## Starting the nmos-registration service
--------------------------------------
The registration service is controlled by a .json config file that sets the ports for web and service access.

user@vm2:~/nmos-dpb/Development/build$ more reg_config.json <br />
{ <br />
    "logging_level": 0, <br />
    "domain": "local.", <br />
    "http_port": 8000,
    "events_ws_port": 8001, <br />
    "label": "vm3-reg" <br />
} <br />

user@vm2:~/nmos-dpb/Development/build$ ./nmos-cpp-registry reg_config.json


## Starting the nmos-node service
------------------------------
The node service is also controlled by a .json config file that sets the ports for web, service and details of the senders and receivers to be created.

user@vm2:~/nmos-dpb/Development/build$ more config.json <br />
{ <br />
    "logging_level": 0, <br />
    "domain": "local.", <br />
    "http_port": 8080, <br />
    "events_ws_port": 8081, <br />
    "label": "vm2", <br />
    "how_many": 0, <br />
    "frame_rate": { "numerator": 25, "denominator": 1 }, <br />
    "interlace_mode": false, <br />
    "smpte2022_7": false, <br />
    "OGC_remote": true, <br />
    "remote_address": "192.168.10.160", <br />
    "remote_int_name": "eno1",
    "remote_mac": "00-00-00-00-00-16",
    "how_many_senders": 2, <br />
    "senders_list": [ <br />
      { "label": "Camera31", "transport":"rtp_ucast", "payload": "video", "format": "raw", "dst_ip": "192.168.10.170", "dst_port": 5034 }, <br />
      { "label": "Camera31", "transport":"rtp_ucast", "payload": "audio", "format": "L24", "dst_ip": "192.168.10.170", "dst_port": 5036 } <br />
    ], <br />
    "how_many_receivers": 2, <br />
    "receivers_list": [ <br />
      { "label": "Camera2", "transport":"rtp_ucast", "payload": "video", "format": "raw" }, <br />
      { "label": "Camera2", "transport":"rtp_ucast", "payload": "audio", "format": "L24" } <br />
    ] <br />
}

The remote_address fields set the IP, MAC and name of the remote media device. <br />
The how_many_sender field sets the number of senders to be created. <br />
The how_many_receivers field sets the number of receivers to be created. <br />
The sender_list fields set the parameters for each sender. <br />
The receivers_list fields set the parameters for each sender. <br />
Note: For rtp_ucast, the sender dst_ip must match the receiver remote_address ip. <br />

user@vm2:~/nmos-dpb/Development/build$ ./nmos-cpp-node config.json


## Accessing the web interfaces
----------------------------
One nmos-cpp-registry service registers multiple nmos-cpp-node services. Using the above .json config files, the web interfaces can be accessed in a web browser at using:

http://<vm2 ip address>:8000/		# Registration service <br />
    Access-Control-Allow-Origin: * <br />
    Access-Control-Expose-Headers: Content-Length, Server-Timing, Timing-Allow-Origin, Vary <br />
    Content-Length: 219 <br />
    Content-Type: application/json <br />
    Server-Timing: proc;dur=3.693 <br />
    Timing-Allow-Origin: * <br />
    Vary: Accept <br />

[<br />
    "admin/",<br />
    "log/",<br />
    "schemas/",<br />
    "settings/",<br />
    "x-dns-sd/",<br />
    "x-nmos/"<br />
]<br />

http://<vm2 ip address>:8080/		# Node service <br />
    Access-Control-Allow-Origin: *b= <br />
    Access-Control-Expose-Headers: Content-Length, Server-Timing, Timing-Allow-Origin, Vary <br />
    Content-Length: 151 <br />
    Content-Type: application/json <br />
    Server-Timing: proc;dur=5.05 <br />
    Timing-Allow-Origin: * <br />
    Vary: Accept <br />

[ <br />
    "log/", <br />
    "settings/", <br />
    "x-manifest/", <br />
    "x-nmos/" <br />
] <br />

## Current limitations
-------------------
The modified versions of nmos-cpp allows multiple types of sender and receiver parameters to be programmed. However:
1.	The receiver destination port is still hard code (which can be ignored)

2.	The registration service has been modified to register multiple format (encoding types) e.g.  <br />
    Video, “format”: "raw",  "H264", "vc2", "H265", "jxsv" <br />
    Audio, “format”: "L24",  "L20", "L16", "L8", "mp3", "aac" <br />

    However the Ctrl App (nmos-js) will only allow the connection of "raw" video and "Lxx" types.
