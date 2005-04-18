
import ex1
import async
import socket

def cb(err,res):
    print "err=", err, "& res=", res


sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
sock.connect (('127.0.0.1', 3000))
fd = sock.fileno ()

print "file descriptor is", fd

x = async.arpc.axprt_stream (fd)
cli = async.arpc.aclnt (x, ex1.foo_prog_1 ())

f = ex1.foo_t ()
f.x = 'this is a test string'
f.xx = 1010


cli.call (ex1.FOO_FUNC, f, cb)

async.core.amain ()

