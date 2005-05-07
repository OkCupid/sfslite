
import ex1
import async
import socket
import sys
import signal

active_srvs = [];

def dispatch (srv, sbp):
    print "in dispatch"
    if sbp.eof ():
        print "EOF !!"
        active_srvs.remove (srv)
        return
    print "procno=", sbp.proc ()
    sbp.getarg ().warn ()
    if sbp.proc () == ex1.FOO_NULL:
        sbp.reply (None)
    elif sbp.proc () == ex1.FOO_BAR:
        bar = sbp.getarg ()
        s = 0
        for i in bar.y:
            print i
            s += i
        f = ex1.foo_t ()
        f.x = 'the sum is ' + `s`
        f.xx = s
        sbp.reply (f)
    else:
        sbp.reject (async.arpc.PROC_UNAVAIL)


def newcon(sock):
    print "calling newcon"
    nsock, addr = sock.accept ()
    print "accept returned; fd=", `nsock.fileno ()`
    print "new connection accepted", addr
    x = async.arpc.axprt_stream (nsock.fileno (), nsock)
    print "returned from axprt_stream init"
    srv = async.arpc.asrv (x, ex1.foo_prog_1 (),
                           lambda y : dispatch (srv, y))
    print "returned from asrv init"
    active_srvs.append (srv)

port = 3000
if len (sys.argv) > 1:
    port = int (sys.argv[1])

sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
sock.bind (('127.0.0.1', port))
sock.listen (10);
async.core.fdcb (sock.fileno (), async.core.selread, lambda : newcon (sock))

async.core.sigcb (signal.SIGINT, sys.exit)
signal.signal (signal.SIGINT, sys.exit)

async.core.amain ()

