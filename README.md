# An NMOS C++ Implementation [![Build Status](https://github.com/sony/nmos-cpp/workflows/build-test/badge.svg)][build-test]
[build-test]: https://github.com/sony/nmos-cpp/actions?query=workflow%3Abuild-test

## Introduction

This repository contains an implementation of the [AMWA Networked Media Open Specifications](https://specs.amwa.tv/nmos/) in C++, [licensed](LICENSE) under the terms of the Apache License 2.0.

- [AMWA IS-04 NMOS Discovery and Registration Specification](https://specs.amwa.tv/is-04/)
- [AMWA IS-05 NMOS Device Connection Management Specification](https://specs.amwa.tv/is-05/)
- [AMWA IS-07 NMOS Event & Tally Specification](https://specs.amwa.tv/is-07/)
- [AMWA IS-08 NMOS Audio Channel Mapping Specification](https://specs.amwa.tv/is-08/)
- [AMWA IS-09 NMOS System Parameters Specification](https://specs.amwa.tv/is-09/) (originally defined in JT-NM TR-1001-1:2018 Annex A)
- [AMWA IS-10 NMOS Authorization Specification](https://specs.amwa.tv/is-10/)
- [AMWA IS-12 AMWA IS-12 NMOS Control Protocol](https://specs.amwa.tv/is-12/)
- [AMWA IS-14 AMWA IS-14 NMOS Device Configuration Specification](https://specs.amwa.tv/is-14/)
- [AMWA BCP-002-01 NMOS Grouping Recommendations - Natural Grouping](https://specs.amwa.tv/bcp-002-01/)
- [AMWA BCP-002-02 NMOS Asset Distinguishing Information](https://specs.amwa.tv/bcp-002-02/)
- [AMWA BCP-003-01 Secure Communication in NMOS Systems](https://specs.amwa.tv/bcp-003-01/)
- [AMWA BCP-003-02 Authorization in NMOS Systems](https://specs.amwa.tv/bcp-003-02/)
- [AMWA BCP-004-01 NMOS Receiver Capabilities](https://specs.amwa.tv/bcp-004-01/)
- [AMWA BCP-006-01 NMOS With JPEG XS](https://specs.amwa.tv/bcp-006-01/)
- [AMWA BCP-008-01 NMOS Receiver Status](https://specs.amwa.tv/bcp-008-01/)
- [AMWA BCP-008-02 NMOS Sender Status](https://specs.amwa.tv/bcp-008-02/)
- [AMWA MS-05-01 NMOS Control Architecture](https://specs.amwa.tv/ms-05-01/)
- [AMWA MS-05-02 NMOS Control Framework](https://specs.amwa.tv/ms-05-02/)

For more information about AMWA, NMOS and the Networked Media Incubator, please refer to <https://www.amwa.tv/>.

- The [nmos module](Development/nmos) includes implementations of the NMOS Node, Registration and Query APIs, the NMOS Connection API, and so on.
- The [nmos-cpp-registry application](Development/nmos-cpp-registry) provides a simple but functional instance of an NMOS Registration & Discovery System (RDS), utilising the nmos module.
- The [nmos-cpp-node application](Development/nmos-cpp-node) provides an example NMOS Node, also utilising the nmos module.

The [repository structure](Documents/Repository-Structure.md), and the [external dependencies](Documents/Dependencies.md), are outlined in the documentation.
Some information about the overall design of **nmos-cpp** is also included in the [architecture](Documents/Architecture.md) documentation.

### Getting Started With NMOS

The [Easy-NMOS](https://github.com/rhastie/easy-nmos) starter kit allows the user to launch a simple NMOS setup with minimal installation steps.
It relies on a containerized nmos-cpp build to provide an NMOS Registry and a virtual NMOS Node in a Docker Compose network, along with the AMWA NMOS Testing Tool and supporting services.

Alternatively, it is possible to install a pre-built package for many platforms.
See the instructions for [installing with Conan](Documents/Installation-with-Conan.md).

### Getting Started For Developers

Easy-NMOS is also a great first way to explore the relationship between NMOS services before building nmos-cpp for yourself.

The codebase is intended to be portable, and the nmos-cpp [CMake project](Development/CMakeLists.txt) can be built on at least Linux, Windows and macOS.

After setting up the dependencies, follow these [instructions](Documents/Getting-Started.md) to build and install nmos-cpp on your platform, and run the test suite.

Next, try out the registry and node applications in the [tutorial](Documents/Tutorial.md).

An [nmos-cpp Conan package](https://conan.io/center/recipes/nmos-cpp) is occasionally published at Conan Center Index.

## Agile Development

[<img alt="JT-NM Tested 03/20 NMOS & TR-1001-1" src="Documents/images/jt-nm-tested-03-20-registry.png?raw=true" height="135" align="right"/>](https://jt-nm.org/jt-nm_tested/)

The nmos-cpp applications, like the NMOS Specifications, are intended to be always ready, but steadily developing.
They have been successfully tested in many AMWA Networked Media Incubator workshops, and are used as reference NMOS implementations in the [JT-NM Tested](https://jt-nm.org/jt-nm_tested/) programme.
Several vendors have deployed JT-NM Tested badged products, using nmos-cpp, to their customers.

### Build Status

The following configurations, defined by the [build-test](.github/workflows/src/build-test.yml) jobs, are built and unit tested automatically via continuous integration.

| Platform | Version                   | Build Options                      | Test Options                                                       |
|----------|---------------------------|------------------------------------|--------------------------------------------------------------------|
| Linux    | Ubuntu 24.04 (GCC 13.2.0) | Avahi                              | Secure Communications<br/>Multicast DNS-SD                         |
| Linux    | Ubuntu 24.04 (GCC 13.2.0) | Avahi                              | Secure Communications<br/>IS-10 Authorization<br/>Multicast DNS-SD |
| Linux    | Ubuntu 22.04 (GCC 11.2.0) | Avahi                              | Secure Communications<br/>IS-10 Authorization<br/>Multicast DNS-SD |
| Linux    | Ubuntu 22.04 (GCC 11.2.0) | Avahi                              | Secure Communications<br/>IS-10 Authorization<br/>Unicast DNS-SD   |
| Linux    | Ubuntu 22.04 (GCC 11.2.0) | mDNSResponder                      | Secure Communications<br/>IS-10 Authorization<br/>Multicast DNS-SD |
| Windows  | Server 2022 (VS 2022)     | Bonjour (mDNSResponder), ASIO      | Secure Communications<br/>Multicast DNS-SD                         |
| Windows  | Server 2022 (VS 2022)     | Bonjour (mDNSResponder), ASIO      | Secure Communications<br/>IS-10 Authorization<br/>Multicast DNS-SD |
| Windows  | Server 2022 (VS 2022)     | Bonjour (mDNSResponder), WinHTTP   | Secure Communications<br/>Multicast DNS-SD                         |
| Windows  | Server 2022 (VS 2022)     | Bonjour (mDNSResponder), WinHTTP   | Secure Communications<br/>IS-10 Authorization<br/>Multicast DNS-SD |

The [AMWA NMOS API Testing Tool](https://github.com/AMWA-TV/nmos-testing) is automatically run against the APIs of the **nmos-cpp-node** and **nmos-cpp-registry** applications.

**Test Suite/Status:**
[![BCP-003-01][BCP-003-01-badge]][BCP-003-01-sheet]
[![BCP-006-01-01][BCP-006-01-01-badge]][BCP-006-01-01-sheet]
[![BCP-008-01-01][BCP-008-01-01-badge]][BCP-008-01-01-sheet]
[![BCP-008-02-01][BCP-008-02-01-badge]][BCP-008-02-01-sheet]
[![IS-04-01][IS-04-01-badge]][IS-04-01-sheet]
[![IS-04-02][IS-04-02-badge]][IS-04-02-sheet]
[![IS-04-03][IS-04-03-badge]][IS-04-03-sheet]
[![IS-05-01][IS-05-01-badge]][IS-05-01-sheet]
[![IS-05-02][IS-05-02-badge]][IS-05-02-sheet]
[![IS-07-01][IS-07-01-badge]][IS-07-01-sheet]
[![IS-07-02][IS-07-02-badge]][IS-07-02-sheet]
[![IS-08-01][IS-08-01-badge]][IS-08-01-sheet]
[![IS-08-02][IS-08-02-badge]][IS-08-02-sheet]
[![IS-09-01][IS-09-01-badge]][IS-09-01-sheet]
[![IS-09-02][IS-09-02-badge]][IS-09-02-sheet]
[![IS-12-01][IS-12-01-badge]][IS-12-01-sheet]
[![IS-14-01][IS-14-01-badge]][IS-14-01-sheet]

[BCP-003-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/BCP-003-01.svg
[BCP-006-01-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/BCP-006-01-01.svg
[BCP-008-01-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/BCP-008-01-01.svg
[BCP-008-02-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/BCP-008-02-01.svg
[IS-04-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-04-01.svg
[IS-04-02-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-04-02.svg
[IS-04-03-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-04-03.svg
[IS-05-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-05-01.svg
[IS-05-02-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-05-02.svg
[IS-07-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-07-01.svg
[IS-07-02-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-07-02.svg
[IS-08-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-08-01.svg
[IS-08-02-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-08-02.svg
[IS-09-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-09-01.svg
[IS-09-02-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-09-02.svg
[IS-12-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-12-01.svg
[IS-14-01-badge]: https://raw.githubusercontent.com/sony/nmos-cpp/badges/IS-14-01.svg
[BCP-003-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=468090822
[BCP-006-01-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit?gid=1835994800
[BCP-008-01-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit?gid=1290589116
[BCP-008-02-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit?gid=582384563
[IS-04-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=0
[IS-04-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1838684224
[IS-04-03-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1174955447
[IS-05-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=517163955
[IS-05-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=205041321
[IS-07-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=828991990
[IS-07-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=367400040
[IS-08-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=776923255
[IS-08-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1558470201
[IS-09-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=919453974
[IS-09-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=2135469955
[IS-12-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit?gid=1026699230
[IS-14-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit?gid=342707873

### Recent Activity

The implementation is designed to be extended. Development is ongoing, following the evolution of the NMOS specifications in the AMWA Networked Media Incubator.

Recent activity on the project (newest first):

- Added support for IS-14 NMOS Device Configuration
- Added support for BCP-008-01 Receiver Status Monitoring
- Added support for BCP-008-02 Sender Status Monitoring
- Added support for IS-12 NMOS Control Protocol
- Update to Conan 2; Conan 1.X is no longer supported
- Added support for IS-10 Authorization
- Added support for HSTS and OCSP stapling
- Added support for BCP-006-01 v1.0-dev, which can be demonstrated with **nmos-cpp-node** by using `"video_type": "video/jxsv"`
- Updates to the GitHub Actions build-test workflow for better coverage of platforms and to include unicast DNS-SD tests
- Regular Conan Center Index releases (see [nmos-cpp recipe](https://github.com/conan-io/conan-center-index/blob/master/recipes/nmos-cpp))
- Update to RQL implementation to support advanced queries for string values containing '.'
- Improvements to the SDP parser/generator
- Improvements to Conan/CMake build, including updates to preferred version of dependencies such as Boost and OpenSSL
- Prepared a basic Conan recipe for building nmos-cpp, in ~[Sandbox/conan-recipe](Sandbox/conan-recipe)~
- Refactored the CMake build to make it easier to use nmos-cpp from another project, demonstrated by [Sandbox/my-nmos-node](Sandbox/my-nmos-node)
- Added support for BCP-004-01 Receiver Capabilities
- Switched CI testing to run the nmos-cpp applications and the AMWA NMOS Testing Tool with secure communication (TLS) enabled, as per BCP-003-01
- Added support for the IS-08 Channel Mapping API
- JT-NM Tested 03/20 badge
- Switched Continous Integration to GitHub Actions and added Windows and macOS to the tested platforms
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
- Some documentation about the overall design of nmos-cpp for developers
- An implementation of the Connection API
- A fix for a potential memory leak
- An SDP parser/generator (to/from a JSON representation)
- JSON Schema validation in the Registration API and the Query API
- Cross-platform build support using CMake
- An initial release of the **nmos-cpp-node** example application
- Back-end enhancements as part of the NMOS Scalability Activity

## Contributing

We welcome bug reports, feature requests and contributions to the implementation and documentation.
Please have a look at the simple [Contribution Guidelines](CONTRIBUTING.md).

Thank you for your interest!

![This project was formerly known as sea-lion.](Documents/images/sea-lion.png?raw=true)
