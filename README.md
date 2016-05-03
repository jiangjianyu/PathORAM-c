PathOram
=======
PathOram was proposed by Emil Stefanov, which is a pretty simple construction of oram protocol.

Following researches have been done to improve its online bandwidth.

This project is based on 
    Constants Count: Practical Improvements to Oblivious RAM
    by Ling Ren etc.


Install
-------
Prerequires : libsodium cmake make

    cmake ./
    make

Usage
-------
    usage: pathoram -c config_file -m [server|client] -b [host] -p port -d [start|stop|restart] -v

Client mode is used to test a server