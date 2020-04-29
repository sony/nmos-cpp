# An NMOS C++ Implementation [![Build Status](https://travis-ci.org/sony/nmos-cpp.svg?branch=master)][Travis-CI] [![Build Status](https://github.com/sony/nmos-cpp/workflows/build-test/badge.svg)][build-test]
[Travis-CI]: https://travis-ci.org/sony/nmos-cpp
[build-test]: https://github.com/sony/nmos-cpp/actions?query=workflow%3Abuild-test

## Introduction

This repository contains an implementation of the [AMWA Networked Media Open Specifications](https://amwa-tv.github.io/nmos) in C++, [licensed](LICENSE) under the terms of the Apache License 2.0.

- [AMWA IS-04 NMOS Discovery and Registration Specification](https://amwa-tv.github.io/nmos-discovery-registration)
- [AMWA IS-05 NMOS Connection Management Specification](https://amwa-tv.github.io/nmos-device-connection-management)
- [AMWA IS-07 NMOS Event & Tally Specification](https://amwa-tv.github.io/nmos-event-tally)
- [AMWA IS-09 NMOS System Specification](https://amwa-tv.github.io/nmos-system) (originally defined in JT-NM TR-1001-1:2018 Annex A)
- [AMWA BCP-002-01 NMOS Grouping Recommendations](https://amwa-tv.github.io/nmos-grouping) - Natural Grouping
- [AMWA BCP-003-01 NMOS API Security Recommendations](https://amwa-tv.github.io/nmos-api-security) - Securing Communications

For more information about AMWA, NMOS and the Networked Media Incubator, please refer to http://amwa.tv/.

- The [nmos module](Development/nmos) includes implementations of the NMOS Node, Registration and Query APIs, the NMOS Connection API, and so on.
- The [nmos-cpp-registry application](Development/nmos-cpp-registry) provides a simple but functional instance of an NMOS Registration & Discovery System (RDS), utilising the nmos module.
- The [nmos-cpp-node application](Development/nmos-cpp-node) provides an example NMOS Node, also utilising the nmos module.

The [repository structure](Documents/Repository-Structure.md), and the [external dependencies](Documents/Dependencies.md), are outlined in the documentation.
Some information about the overall design of **nmos-cpp** is also included in the [architecture](Documents/Architecture.md) documentation.

### Getting Started

The codebase is intended to be portable, and the nmos-cpp [CMake project](Development/CMakeLists.txt) can be built on at least Windows and Linux.

After setting up the dependencies, follow these [instructions](Documents/Getting-Started.md) to build nmos-cpp on your platform, and run the test suite.

Next, try out the registry and node applications in the [tutorial](Documents/Tutorial.md).

## Active Development

The implementation is functional and has been used successfully in several Networked Media Incubator workshop "plug-fests", interoperating with other NMOS implementations, and is being used to build NMOS support for several products.

### Build Status

The following configurations are built and unit tested automatically via continuous integration.

| Build Name                     | Status                                 |
|--------------------------------|----------------------------------------|
| Linux (Ubuntu 14.04/GCC 4.8.4) | [![Build 1][Build-1-badge]][Travis-CI] |
| Linux (Ubuntu 18.04/GCC 7.4.0) | [![Build 2][Build-2-badge]][Travis-CI] |

[Build-1-badge]: https://travis-matrix-badges.herokuapp.com/repos/sony/nmos-cpp/branches/master/1
[Build-2-badge]: https://travis-matrix-badges.herokuapp.com/repos/sony/nmos-cpp/branches/master/2

The [AMWA NMOS API Testing Tool](https://github.com/AMWA-TV/nmos-testing) is automatically run against the APIs of the **nmos-cpp-node** and **nmos-cpp-registry** applications.

**Test Suite/Status:**
[![IS-04-01][IS-04-01-badge]][IS-04-01-sheet]
[![IS-04-02][IS-04-02-badge]][IS-04-02-sheet]
[![IS-04-03][IS-04-03-badge]][IS-04-03-sheet]
[![IS-05-01][IS-05-01-badge]][IS-05-01-sheet]
[![IS-05-02][IS-05-02-badge]][IS-05-02-sheet]
[![IS-07-01][IS-07-01-badge]][IS-07-01-sheet]
[![IS-07-02][IS-07-02-badge]][IS-07-02-sheet]
[![IS-09-01][IS-09-01-badge]][IS-09-01-sheet]
[![IS-09-02][IS-09-02-badge]][IS-09-02-sheet]

[IS-04-01-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D1VrCPcYeTs5uoBgECxbfuWbbhJZpbHcPy
[IS-04-02-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D14vgZF4CSx2oayEAbeNFGiHmPW95HKMXt
[IS-04-03-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D16616xSByskr3PbeqhnCcNTjfJcDdzUav
[IS-05-01-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D1tW25Xim9LymIvPXnxM5taGmlLVsXa71p
[IS-05-02-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D1MkQNv8v2r0ydB1mQ55k-pktlzE8LZ3g9
[IS-07-01-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D1XQuAN13xAQ81G_Eokj6AAYv5kMInPXkZ
[IS-07-02-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D16t7XCmsQaOw5eEqq6yuuy1U9I3J-9zN9
[IS-09-01-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D16t7ncRp3SbHHoftQY-RBi2NFC283fOTn
[IS-09-02-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdrive.google.com%2Fuc%3Fexport%3Ddownload%26id%3D1f4FHD6vI1LotF7Sm8U6tmNp58seW9397
[IS-04-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=0
[IS-04-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1838684224
[IS-04-03-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1174955447
[IS-05-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=517163955
[IS-05-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=205041321
[IS-07-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=828991990
[IS-07-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=367400040
[IS-09-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=919453974
[IS-09-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=2135469955

### Recent Activity

The implementation is incomplete in some areas. Development is ongoing, tracking the evolution of the NMOS specifications in the AMWA Networked Media Incubator.

Recent activity on the project (newest first):

- Extended the **nmos-cpp-node** to include mock senders/receivers of audio and ancillary data and offer some additional configuration settings
- Simplified the build process to use Conan by default to download most of the dependencies
- Added support in the Node implementation for discovery of, and interaction with, a System API, as required for compliance with TR-1001-1
- Changed the implementation of `nmos::tai_clock` with the effect that it may no longer be monotonic
- Added a minimum viable LLDP implementation (enabled by a CMake configuration option) to support sending and receiving the IS-04 v1.3 additional network data for Nodes required by IS-06
- Update the IS-05 schemas to correct an unfortunate bug in the IS-05 v1.1 spec (see [AMWA-TV/nmos-device-connection-management#99](https://github.com/AMWA-TV/nmos-device-connection-management/pull/99))
- Attempt to determine the DNS domain name automatically if not explicitly specified, for TR-1001-1
- Travis CI integration
- Updates for resolutions of specification issues in IS-04 v1.3 and IS-05 v1.1 final drafts
- Experimental support for human-readable HTML rendering of NMOS responses
- Experimental support for the rehomed (work in progress) IS-09 System API (originally defined in JT-NM TR-1001-1:2018 Annex A)
- IS-07 Events API and Events WebSocket API implementation and updated nmos-cpp-node example
- Experimental support for secure communications (HTTPS, WSS)
- Bug fixes (with test cases added to the [AMWA NMOS API Testing Tool](https://github.com/AMWA-TV/nmos-testing))
- Support for running nmos-cpp applications with forward/reverse proxies
- Experimental support for JT-NM TR-1001-1 System API
- Instructions for cross-compiling for the Raspberry Pi
- Instructions for running the official AMWA NMOS API Testing Tool
- Updates to build instructions and required dependencies
- Simpler creation/processing of the types of SDP files required to support ST 2110 and ST 2022-7
- Simpler run-time configuration of the **nmos-cpp-node** and **nmos-cpp-registry** settings
- Some documentation about the overall design of **nmos-cpp** for developers
- An implementation of the Connection API
- A fix for a potential memory leak
- An SDP parser/generator (to/from a JSON representation)
- JSON Schema validation in the Registration API and the Query API
- Cross-platform build support using CMake
- An initial release of the **nmos-cpp-node** example application
- Back-end enhancements as part of the NMOS Scalability Activity

## Contributing

We welcome bug reports, feature requests and contributions to the implementation and documentation. Please have a look at the simple [Contribution Guidelines](CONTRIBUTING.md).

Thank you for your interest!

![This project was formerly known as sea-lion.](Documents/images/sea-lion.png?raw=true)
