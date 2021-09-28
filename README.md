## NMOS C++ Implementation for the 5G-Records Project

Work in progress for remote proxy nmos node.

## Introduction

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

user@vm2:~$ sudo apt-get install -y python-markupsafe	#May or may not be needed depending on app versions

user@vm2:~$ pip3 install conan

user@vm2:~$ sudo cp -r .local/bin/* /usr/local/bin/

user@vm2:~$ ls /usr/local/bin/ <br />
bottle.py  conan_build_info  tqdm  conan  conan_server __pycache__  

user@vm2:~$ conan --version <br />
Conan version 1.33.0

For registration, a MDNS responder must be installed and configured

user@vm2:~$ wget https://opensource.apple.com/tarballs/mDNSResponder/mDNSResponder-878.270.2.tar.gz

user@vm2:~$ tar -xzvf mDNSResponder-878.270.2.tar.gz

user@vm2:~/mDNSResponder-878.270.2/mDNSPosix$ make os=linux

user@vm2:~/mDNSResponder-878.270.2/mDNSPosix$ sudo make os=linux install

Check the nsswitch.conf file. <br />
If 'mdns' does not already appear on the "hosts:" line, then add it right before 'dns' <br />

root@vm2:~$ cp -f /etc/nsswitch.conf /etc/nsswitch.conf.pre-mdns
```
root@vm2:~$ sed -e '/mdns/!s/^\(hosts:.*\)dns\(.*\)/\1mdns dns\2/' /etc/nsswitch.conf.pre-mdns > /etc/nsswitch.conf
```
root@vm2:~$ cp nss_mdns.conf.5 /usr/share/man/man5/nss_mdns.conf.5

root@vm2:~$ chmod 444 /usr/share/man/man5/nss_mdns.conf.5

root@vm2:~$ cp libnss_mdns.8 /usr/share/man/man8/libnss_mdns.8

root@vm2:~$ chmod 444 /usr/share/man/man8/libnss_mdns.8 /lib/libnss_mdns.so.2 /etc/nss_mdns.conf /usr/share/man/man5/nss_mdns.conf.5 /usr/share/man/man8/libnss_mdns.8

user@vm2:~/mDNSResponder-878.270.2/mDNSPosix$ more /etc/nsswitch.conf <br />
&#35; /etc/nsswitch.conf <br />
&#35; Example configuration of GNU Name Service Switch functionality. <br />
&#35; If you have the `glibc-doc-reference' and `info' packages installed, try: <br />
&#35; `info libc "Name Service Switch"' for information about this file. <br />
passwd:         files systemd <br />
group:          files systemd <br />
shadow:         files <br />
gshadow:        files <br />
hosts:          files mdns4_minimal [NOTFOUND=return] mdns dns <br />
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

The registration service is controlled by a .json config file that sets the ports for web and service access.

user@vm3:~/nmos-dpb/Development/build$ more reg_config.json <br />
```json
{ 
    "logging_level": 0,
    "domain": "local.",
    "http_port": 8000,
    "events_ws_port": 8001, 
    "label": "vm3-reg"
} 
```
user@vm3:~/nmos-dpb/Development/build$ ./nmos-cpp-registry reg_config.json


## Starting the nmos-node service

The node service is also controlled by a .json config file that sets the ports for web, service and details of the senders and receivers to be created. For websockets, both a sender and receiver are created for each specified websocket.

user@vm2:~/nmos-dpb/Development/build$ more config.json <br />
```json
{
    "logging_level": 0,
    "domain": "local.",
    "http_port": 8080,
    "events_ws_port": 8081,
    "label": "vm2", 
    "how_many": 0, 
    "frame_rate": { "numerator": 25, "denominator": 1 }, 
    "interlace_mode": false, 
    "smpte2022_7": false, 
    "OGC_remote": true, 
    "remote_address": "192.168.10.160",
    "remote_int_name": "eno1",
    "remote_mac": "00-00-00-00-00-16",
    "how_many_senders": 4, 
    "senders_list": [ 
      { "label": "Camera1", "transport":"rtp_ucast", "payload": "video", "format": "raw", "dst_ip": "192.168.10.170", "dst_port": 7004 }, 
      { "label": "Camera1", "transport":"rtp_ucast", "payload": "audio", "format": "L16", "dst_ip": "192.168.10.170", "dst_port": 7006 }, 
	  { "label": "Camera2", "transport":"rtp_ucast", "payload": "video", "format": "H264", "dst_ip": "192.168.10.170", "dst_port": 7008 }, 
	  { "label": "Camera2", "transport":"rtp_ucast", "payload": "audio", "format": "aac", "dst_ip": "192.168.10.170", "dst_port": 7010 } 
    ], 
    "how_many_receivers": 4, 
    "receivers_list": [ 
      { "label": "Camera3", "transport":"rtp_ucast", "payload": "video", "format": "raw", "dst_port": 6004 }, 
      { "label": "Camera3", "transport":"rtp_ucast", "payload": "audio", "format": "L24", "dst_port": 6006 }, 
	  { "label": "Camera3", "transport":"rtp_ucast", "payload": "video", "format": "vc2", "dst_port": 6008 }, 
	  { "label": "Camera3", "transport":"rtp_ucast", "payload": "audio", "format": "aac", "dst_port": 6010 } 
    ],
    "how_many_wsocket": 4,
    "wsocket_list": [
      { "label": "Coder1-temp", "ws_type":"temperature"},
      { "label": "Camera1-tally", "ws_type":"boolean"},
      { "label": "ParamLabel", "ws_type":"param_string"},
      { "label": "ParamValues", "ws_type":"param_values"}
    ] 
}
```
user@vm3:~/nmos-dpb/Development/build$ more config.json
```json
{ 
    "logging_level": 0, 
    "domain": "local.", 
    "http_port": 8080, 
    "events_ws_port": 8081,
    "label": "vm3", 
    "how_many": 0, 
    "frame_rate": { "numerator": 25, "denominator": 1 }, 
    "interlace_mode": false, 
    "smpte2022_7": false,
    "OGC_remote": true, 
    "remote_address": "192.168.10.170", 
    "remote_int_name": "eno1",
    "remote_mac": "00-00-00-00-00-17",
    "how_many_senders": 4, 
    "senders_list": [ 
      { "label": "Camera3", "transport":"rtp_ucast", "payload": "video", "format": "raw", "dst_ip": "192.168.10.160", "dst_port": 6004 }, 
      { "label": "Camera3", "transport":"rtp_ucast", "payload": "audio", "format": "L24", "dst_ip": "192.168.10.160", "dst_port": 6006 }, 
	  { "label": "Camera4", "transport":"rtp_ucast", "payload": "video", "format": "vc2", "dst_ip": "192.168.10.160", "dst_port": 6008 }, 
	  { "label": "Camera4", "transport":"rtp_ucast", "payload": "audio", "format": "aac", "dst_ip": "192.168.10.160", "dst_port": 6010 } 
    ], 
    "how_many_receivers": 4, 
    "receivers_list": [ 
      { "label": "Camera1", "transport":"rtp_ucast", "payload": "video", "format": "raw", "dst_port": 7004 }, 
      { "label": "Camera1", "transport":"rtp_ucast", "payload": "audio", "format": "L16", "dst_port": 7006 }, <
	  { "label": "Camera2", "transport":"rtp_ucast", "payload": "video", "format": "H264", "dst_port": 7008 }, 
	  { "label": "Camera2", "transport":"rtp_ucast", "payload": "audio", "format": "aac", "dst_port": 7010 } <
    ],
        "how_many_wsocket": 4,
    "wsocket_list": [
      { "label": "Coder1-temp", "ws_type":"temperature"},
      { "label": "Camera1-tally", "ws_type":"boolean"},
      { "label": "ParamLabel", "ws_type":"param_string"},
      { "label": "ParamValues", "ws_type":"param_values"}
    ]
}
```
Due to the json array implementation in the nmos-cpp code, if no senders, receivers or websockets are needed, this lists needed to contain at least 1 item. The item can be empty. e.g.
```json
    "how_many_senders": 0,
    "senders_list": [
      { "label": "", "":"rtp_ucast", "payload": "", "format": "", "dst_ip": "", "dst_port": 0 }
    ],
    "how_many_receivers": 0,
    "receivers_list": [
      { "label": "", "transport":"", "payload": "", "format": "", "dst_port": 0 }
    ],
    "how_many_wsocket": 0,
    "wsocket_list": [
      { "label": "", "ws_type":""}
    ]
```

The remote_address fields set the IP, MAC and name of the remote media device. <br />
The how_many_sender field sets the number of senders to be created. <br />
The how_many_receivers field sets the number of receivers to be created. <br />
The sender_list fields set the parameters for each sender. <br />
The receivers_list fields set the parameters for each receiver. <br />
The how_many_wsocket field sets the number of both web spockets senders and receivers to be created. <br />
Note: <br />
For rtp_ucast, the sender dst_ip and dst_port must match the receiver remote_address ip and dst_port. <br />
For all senders and receivers that connect the payload and format must match. <br />

user@vm2:~/nmos-dpb/Development/build$ ./nmos-cpp-node config.json


## Accessing the web interfaces

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

The modified versions of nmos-cpp allows multiple types of sender and receiver parameters to be programmed. However:
1.	The video resolution is hard coded to HD (1920x1080), scan can be set to interlaced/progressive and frame rate can be programmed.

2.	The registration service has been modified to register multiple format (encoding types) e.g.  <br />
    Video, “format”: "raw",  "H264", "vc2", "H265", "jxsv" <br />
    Audio, “format”: "L24",  "L20", "L16", "L8", "aac", "mp2", "mp3", "m4a", "opus"  <br />
<br />
    The registration service and Ctrl App (nmos-js) will only allow the connection of each media types, but lower level details are hard coded. <br />



