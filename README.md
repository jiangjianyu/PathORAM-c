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
    sudo make install

Usage
-------
#####For Server Side

    usage: pathoram -c config_file -m [server|client] -b [host] -p port -d [start|stop|restart] -v

#####For Client Side:

Create test.c, and write

    #include <pathoram.h>

    client_ctx ctx;
    oram_client_args ars;

    //Init varibles, this is the default values,
    //Could be ignore
    ar.host = "127.0.0.1"
    ar.port = 31111;
    ar.save_file = "ORAM_CLIENT.meta"
    ar.load_file = "ORAM_CLIENT.meta"
    sprintf(ar.key,"ORAM");

    //This varible decides if server will re-init after init
    int re_init;

    //Init client
    client_init(&ctx, &arg);

    //Create client
    client_create(&ctx, re_init);

    //Load client
    client_load(&ctx, re_init);

    //Save client
    client_save(&ctx);

    //Acess data at address
    oblivious_access(address, oram_access_op, data[], &ctx);

Compile test.c

    gcc -o test test.c -lm -lsodium -lpathoram