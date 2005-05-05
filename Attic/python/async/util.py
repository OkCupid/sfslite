
__all__ = [ "xdrcp" ]

def xdrcp(x):
    n = x.__new__ (type (x))
    n.str2xdr (x.xdr2str ())
    return n
