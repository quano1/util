# util
Utility libraries

o 4 file descriptors (fd)
    - console
    - file
    - udp socket
    - <reserve>

o log type
    - trace
    - info
    - fatal
    - debug

o functionalities
    - enable/disable log type per fd at runtime via POSIX pipe.
    - configure udp socket via POSIX pipe: host, port.
    - a transmitting thread per each fd.

