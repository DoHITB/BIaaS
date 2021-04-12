# BIaaS
Big Integer as a Service

Server implementation for BigOperation (aka BigInteger - https://github.com/DoHITB/BigInteger.c).


## How it works
1. There's an Apache Custom Module (`mainh.c`), that gets each ".calc" request
  1. For this, just add `AddHandler calc-handler .calc` on your Apache .htaccess file
2. This `calc-handler` will get the request and write a file on `./f` (folder must exist befor execution). 
  1. There are two `kind` of operations to make:
    1. `makeOperation`, in order to enter an operation
    2. `getResult`, in order to get the result of `makeOperation`.
3. The `BigInteger` service (`plist`) will take the file, calculate, and write the result on `/r` folder (appended with `r_`). 


## makeOperation
You have to follow this rules:

1. File name (`fileName` parameter) may not exceed 126 characters long.
2. File name (`fileName` parameter) may be set.
3. File name (`fileName` parameter) may be unique and non-existing. It will be searched among `/f` and `/r` folders.
4. Operation data (`operation`, `op1` and `op2` parameters) may be informed.
5. Operation data (`op1` and `op2` parameters) may not exceed 1024 length (during beta; on release, will stand up to 4096).
6. Operation data (`operation` parameter) may be only 1 character long and be either `+`, `-`, `/`, `*`, `^`, `s` for adding, subtracting, dividing, multiplying, power, and get root of, respectively.


## getResult
You have to follow this rules

1. File name (`fileName` parameter) may not exceed 126 character long.
2. File name (`fileName` parameter) may be set.
3. File name (`fileName` parameter) may be unique and existing. It will be searched among `/f` and `/r` folders.

Getting a result will erase the `/r` file.


## Starting all the stuff
1. First, you may need to download all source code, including BigInteger (https://github.com/DoHITB/BigInteger.c).
2. Edit your .htaccess file to get the handler
3. Compile `mainh.c` by using `makemain.sh`. You will need `apsx` compiler from Apache developer package. Apache service will restart.
4. Compile `plist` by using `makeplist.sh`. You will need to have all BigInteger source in the same folder.
5. Compile `plistStarter` with gcc. No special needs.
6. Move `plist` and `plistStarter` objects to `/var/www/c`.
7. Create folders `/var/www/c/f` and `/var/www/c/r`.
8. Copy `plistClean.sh` (to keep `/r` folder clean), `plistStarter.sh` (script to run only one instance of `plist`) and `watchplist.sh` (scheduler for `plistStarter`) to `/var/www/c` folder.
9. Grant privileges to executable files.
10. Run `./watchplist.sh` and `./plistClean.sh` from `/var/www/c` folder.
11. Your server is ready.


## plist
This service just opens `/var/www/c/f` folder, iterate over it (storing up to 1024 files), and then treat them, storing results on `/var/www/c/r` folder, appending a `r_` to original file name.

It delegates to `BOperation.c` main launcher to make all calculations.


## mainh
This handler just gets GET data, and writes a file that `plist` can understand.


## Overview

                                   +---------------------------------------------+
                                   |                                             |
    +-------+                 +----+-----------+          +----------------+     |
    |       |  makeOperation  |                |          |                |     |
    |  Web  +---------------->|  calc-handler  +--------->|  /var/www/c/f  |     |
    |       |<----------------+                |          |                |     |
    +---+---+     response    +------------+---+          +----------------+     |
      ^ |                          ^       |                  ^                  |
      | +--------------------------+       |                  |                  |
      |          getResult                 |                  |                  |
      +------------------------------------+                  |                  |
                  response                                    |                  |
                              +--------+                      | read             |
                              |        +----------------------+                  |
                              | plist  |                                         |
                              |        +----------------------+                  |
                              +--------+                      | write            |
                                                              |                  |
                                                              v                  |
                                                          +----------------+     |
                                                          |                |     |
                                                          |  /var/www/c/r  |<----+
                                                          |                |
                                                          +----------------+
