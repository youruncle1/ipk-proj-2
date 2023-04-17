## Changelog
chronologically...

|   hash  |                                      commit message                                       |
| --- | --- |
| 549d5c89a7 | initializied ipkcpd w/ argument parser functionality and makefile |
| b1be4386ed | added UDP connection implementation                                                  |
| 676a6fb05d | added TCP implementation, multithread to go                                   |
| 737cb42afd | implemented multithreaded tcp connections to handle more clients, changed argparser to default host/port/mode values                 |
| 44ea76f797 | 	fixed TCP connection handling for messages that are sent in chunks, added new error message in UDP handler                                          |
| f4e959a75a | small touches to structure of code    |

## Known limitations: 
- This server program is for UNIX-like systems only, as the source code uses headers which are part of POSIX standard.