# Simple Tunnel SSH
## Motivation
* Set up a socket server to maintain a ssh session.
* This can prevent ssh client from exploiting over-connection to the server, which maybe consider as an attack.
* How it works:
  * In request: `(Targe server) <---by ssh---< (tnlssh) <---by socket---< (tnlclient)`
  * The response vice versa.

## How to use ?
* Firstly, run `make`. It'll generate `tnlssh`, `example` and `libtnlc.so`.
* `tnlssh` is the server that runs **socket server** and **ssh client**.
* `example` is an example that shows how to use `tnlclient` with `libtnlc.so`.
* Run `tnlssh <password> <username> <ip> <port>` to start the server.

## MISC
* These codes are based on `libssh` which you can find [here](https://api.libssh.org/stable/index.html).
* `tnlssh` will send an ignored message of ssh every 10 minutes to keep session.
* After calling `tnlclient`, the return value must be free since it is allocated by `realloc`.