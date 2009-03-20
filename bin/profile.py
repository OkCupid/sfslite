#!/usr/bin/env python

import os
import sys
import subprocess
import re
import getopt

##=======================================================================

ADDR2LINE = "addr2line"
DOT = "dot"
FILE = "file"

##=======================================================================

class ProfileError (Exception):
    def __init__ (self, m):
        self._msg = m
    def __str__ (self):
        return self._msg

##=======================================================================

def emsg (s, b = None, e = None):
    v = s.split ('\n')
    if b is None:
        b = "** "
    if e is None:
        e = " **"
    for l in v:
        sys.stderr.write ("%s%s%s\n" %( b, l, e));

##=======================================================================

class OutputTypes:

    ##----------------------------------------

    TXT = 1
    DOT = 2
    PNG = 3
    PS = 4

    ##----------------------------------------
    
    @classmethod
    def from_str (self, x):
        try:
            return getattr (self, x.upper ())
        except AttributeError, e:
            raise ValueError, "bad type %s" % x

    ##----------------------------------------

    @classmethod
    def All (self):
        return set ([self.TXT, self.DOT, self.PNG, self.PS] )

##=======================================================================

class Stream:
    """A stream from an error log, with a few additional features for
    ease of parsing."""

    ##----------------------------------------

    def __init__ (self, s):
        self._stream = s
        self._eof = False

    ##----------------------------------------

    def __iter__ (self):
        return self

    ##----------------------------------------

    def next (self):
        r = self._stream.readline ()
        if len(r) == 0:
            self._eof = True
            raise StopIteration
        r = r.strip ()
        return r

    ##----------------------------------------

    def eof (self):
        return self._eof

##=======================================================================

class Line:
    """A line of input from an (SSP) dump in an error log."""

    ##----------------------------------------

    def __init__ (self, prefix, prog, meat):
        self._prefix = prefix
        self._prog = prog
        self._meat = meat

    ##----------------------------------------

    def __str__(self):
        return "%s %s %s" % (self._prefix, self._prog, self._meat)

    ##----------------------------------------

    def meat (self):
        return self._meat

    ##----------------------------------------

    def prefix (self):
        return self._prefix

    ##----------------------------------------

    def prog (self):
        return self._prog

##=======================================================================

def isDoubled (v):
    if len (v) % 2 != 0:
        return False
    hlen = len (v) / 2
    for i in range (hlen):
        if v[i] != v[i+hlen]:
            return False
    return True

##=======================================================================

class Site:
    """A callsite, consisting of a (file,intstr-pointer) pair.  Many
    of these can map to a single symbol/function name."""

    ##----------------------------------------

    def __init__ (self, file, addr):
        self._file = file
        self._addr = addr
        self._node = None
        self._loc = None
        self._funcname = None

    ##----------------------------------------

    def lookupFunctionName (self, p, inln):

        lines = []

        # Note that addr2line's interface is crap, so this function is
        # much more complicated than it should be.  The issue is that 
        # with the -i flag, addr2line will output the callstack for inlined
        # functions, which is up to N frames. Unfortunately, we have no
        # idea how many frames we're going to have to read. To work around
        # it, we'll ask for the answer twice, and stop as soon as we have
        # a repeated call stack. Will fail miserably for inlined 
        # recursion, which I am assuming should not happen... Note to
        # writers of addr2line/binutils: should preface the answer with
        # the number of lines to read.

        if inln:
            p.stdin.write ("0x%x\n0x%x\n" % (self._addr, self._addr))

            go = True
            while go:
                line = p.stdout.readline ().strip ()
                lines += [ line ]
                go = not isDoubled (lines)
        else:
            p.stdin.write ("0x%x\n" % self._addr)
            for i in range (2):
                line = p.stdout.readline ().strip ()
                lines += [ line ]
                
        func = lines[-2]
        loc = lines[-1]
            
        if loc == "??:0" or func == "??":
            print "XX cannot find symbol at offset 0x%012x in %s" \
                % (self._addr, self._file.name ())
        else:
            self._funcname = func
            self._loc = loc

    ##----------------------------------------

    def funcname (self):
        return self._funcname

    ##----------------------------------------

    def node (self):
        return self._node

    ##----------------------------------------

    def setNode (self, n):
        self._node = n

##=======================================================================

class Node:
    """A node in the graph  --- one for each symbol/function name."""

    ##----------------------------------------

    def __init__ (self, n, id):
        self._name = n
        self._caller_hits = 0
        self._callee_hits = 0
        self._id = id
        self._callees = {}

    ##----------------------------------------

    def name (self): return self._name
    def addCalleeHits (self, h): self._callee_hits += h
    def addCallerHits (self, h): self._caller_hits += h
    def id (self): return self._id

    ##----------------------------------------

    def hitsSelf (self):
        return max (0, self._callee_hits - self._caller_hits)

    ##----------------------------------------

    def hits (self): 
        h = self._callee_hits
        if h == 0:
            h = self._caller_hits
        return h

    ##----------------------------------------

    def dump (self, f):
        print >>f, " %s %s %s" % (self._name, self.hits (), self.hitsSelf ())
        

##=======================================================================

class Edge:
    """An edge in the call graph, consisting of a caller/callee pair."""

    def __init__ (self, caller, callee, hits):
        self._caller = caller
        self._callee = callee
        self._hits = hits

    ##----------------------------------------
        
    def __str__ (self):
        return "%s-%s" % (self._caller.name (), self._callee.name ())

    ##----------------------------------------

    def info (self):
        return "%s %s %d" % (self._caller.name (), self._callee.name (),
                              self._hits)

    ##----------------------------------------

    def dump (self, f):
        print >>f, " %s" % self.info ()

    ##----------------------------------------

    def __iadd__ (self, a):
        self._hits += a._hits
        return self

    ##----------------------------------------
    
    def hits (self): return self._hits
    def caller (self): return self._caller
    def callee (self): return self._callee

##=======================================================================

def my_int (s):
    base = 10
    if s[0:2] == "0x":
        base = 16
    return int (s, base)

##-----------------------------------------------------------------------

def node_sort_fn (a, b):
    return b.hits () - a.hits ()

##-----------------------------------------------------------------------

def edge_sort_fn (a, b):
    return b.hits () - a.hits ()

##=======================================================================

def isExe (p):
    cmd = [ FILE, "-L", p ]
    p = subprocess.Popen (cmd, 
                          stdin = subprocess.PIPE,
                          stdout = subprocess.PIPE,
                          close_fds = True)
    desc = p.stdout.readline ().strip ()
    ret = bool (re.search ("\\bexecutable\\b", desc))
    return ret

##=======================================================================
        
class File:
    """Representation of an .so library or a 'main' object files.
    Keeps track of the file name, the base offset assigned by the dynamic
    loader, and also whether this is the 'main' object file or just
    a linked-in shared library."""

    ##----------------------------------------

    def __init__ (self, name, offset, main, jail):
        self._name = name
        self._offset = offset
        self._jail = jail
        self._main = isExe (self.jname ())

    ##----------------------------------------

    def applyOffset (self, a):
        if not self._main:
            a -= self._offset
        return a

    ##----------------------------------------

    def name (self):
        return self._name

    ##----------------------------------------

    def jname (self):
        if self._jail:
            ret = self._jail + "/" + self._name
        else:
            ret = self._name
        return ret

##=======================================================================

def split_to_ints (l):
    v = l.split ()
    try:
        v = [ my_int (x) for x in v]
    except ValueError:
        v = None
    return v

##=======================================================================

def csym_split (s):
    LINE_LEN = 60
    rxx = re.compile ("^(.*?([<,]|::))(.*)$")
    out = []
    go = True
    while go and len (s) > LINE_LEN:
        prfx = s[0:LINE_LEN]
        m = rxx.match (s[LINE_LEN:])
        if m:
            out += [ prfx + m.group (1) ]
            s = m.group (3)
        else:
            go = False
    out += [ s ]
    return '\\n '.join (out)

##=======================================================================

def toDotParams (lst):
    ret = "[" + ', '.join (['%s="%s"' % p for p in lst ]) + "]"
    return ret

##=======================================================================

class Graph:
    """A graph of an (SSP) run."""

    ##----------------------------------------

    def __init__ (self, lines, serial, props):
        self._serial = serial

        self.initFromStrings (lines, props)

    ##----------------------------------------

    line_rxx = re.compile ("^(e(dge)?|f(ile)?|s(ite)?):\s+(.*)$")

    ##----------------------------------------

    def initFromStrings (self, lines, props):

        files = {}
        sites = {}
        edges = []

        self._begin = lines[0].prefix ()
        self._prog = lines[0].prog ()
        self._end = lines[-1].prefix ()

        for l in lines:
            m = self.line_rxx.match (l.meat ())
            if not m:
                continue

            typ = m.group (1)
            dat = m.group (5)

            # Deal with "file:" and/or "f:"
            # note, only one per line
            if typ[0] == "f":
                v = dat.strip ().split ()
                if len(v) == 4:
                    (id, nm, off, mn) =  \
                        (int (v[0]), v[1], my_int(v[2]), bool (int(v[3])) )
                    files[id] = File (name = nm, offset = off, main = mn,
                                      jail = props.jail ())

            # deal with "site" or "s:", with many triples per line,
            # separate by ";" 
            elif typ[0] == "s":
                
                for trip in dat.split (";"):
                    v = split_to_ints (trip.strip ())
                    if len (v) == 3:
                        sites[v[0]] = v[1:]

            # deal with "edge" or "e:", with many triples per line,
            # separate by ";" 
            elif typ[0] == "e":
                
                for trip in dat.split (";"):
                    v = split_to_ints (trip.strip ())
                    if len (v) == 3:
                        edges.append (tuple(v))

        self._sites = {}
        self._sites_by_file = {}
        self._nodes = {}

        self.initSites (sites, files)
        self.lookupFunctionNames (props)
        self.initNodes ();
        self.initEdges (edges)

    ##----------------------------------------

    def initSites (self, sites, files):
        """ for all call sites, make a new site object, and then
        store them sorted by .so file, so that way we can look
        them all up together."""

        for k in sites.keys ():

            (file_id, addr) = sites[k]
            file = files[file_id]
            addr = file.applyOffset (addr)

            site = Site (file, addr)
            self._sites[k] = site

            v = self._sites_by_file.get (file)
            if v:
                v.append (site)
            else:
                self._sites_by_file[file] = [ site ]

    ##----------------------------------------

    def pct (self, i):
        x = 100.00 * float (i) / float (self._total_samples)
        return "%2.4g%% %d" % (x, i)

    ##----------------------------------------

    def nodeSize (self, i):
        x = max(10, float (i) * 20.00 / float (self._total_samples))
        return x

    ##----------------------------------------

    def penWidth (self, i, d):
        if d is None:
            d = self._total_samples
        x = max (1, float (i) * 8 / float (d))
        return x

    ##----------------------------------------

    def color (self, i, d = None):
        if d is None:
            d = self._total_samples
        r = 255 - i * 200 / d
        return "#ff%2x%2x" % (r,r)

    ##----------------------------------------

    def initNodes (self):
        """Map mutliple call sites to a signle Node object."""
        id = 0
        for v in self._sites.values ():

            # Note that the lookup might have failed...
            funcname = v.funcname()

            if funcname:
                n = self._nodes.get (funcname)
                if not n:
                    n = Node (funcname, id)
                    id += 1
                    self._nodes[funcname] = n
                v.setNode (n)

    ##----------------------------------------

    def initEdges (self, edges):
        """Initialize all edges, as a function of node to node mappings.
        Also, fill in hit counts per node while we're at it."""
        self._edges = {}
        for e in edges:
            hits = e[2]
            caller = self._sites[e[0]].node ()
            callee = self._sites[e[1]].node ()
            if caller and callee:
                callee.addCalleeHits (hits)
                caller.addCallerHits (hits)
                e = Edge (caller = caller, callee = callee, hits = hits)
                e2 = self._edges.get (str(e))
                if e2: e2 += e
                else: self._edges[str(e)] = e
            
    ##----------------------------------------

    def lookupFunctionNames (self, props):

        inln = props.inlining ()

        for f in self._sites_by_file:

            cmd = [ ADDR2LINE ]
            if inln:
                cmd +=  [ "-i" ]
            cmd += [ "-C", "-f", "-e", f.jname () ]

            p = subprocess.Popen (cmd, 
                                  stdin = subprocess.PIPE,
                                  stdout = subprocess.PIPE,
                                  close_fds = True)

            v = self._sites_by_file[f]

            for site in v:
                site.lookupFunctionName (p, inln)

    ##----------------------------------------

    def sort (self):
        v = self._edges.values ()
        v.sort (edge_sort_fn)
        self._sorted_edges = v

        v = self._nodes.values ()
        v.sort (node_sort_fn)
        self._sorted_nodes = v
        self._total_samples = v[0].hits ()

    ##----------------------------------------
        
    def dump (self, f):

        print >>f, "Edges -------------------------------------------"
        for e in self._sorted_edges: 
            e.dump (f)

        print >>f
        print >>f, "Nodes -------------------------------------------"
        
        for n in self._sorted_nodes: 
            n.dump (f)
            
    ##----------------------------------------

    def output (self, props):

        types = props.outputTypes ()

        self.sort ()

        tab = [ ( OutputTypes.TXT, self.outputText),
                ( OutputTypes.DOT, self.outputDot),
                ( OutputTypes.PS,  self.outputPs),
                ( OutputTypes.PNG, self.outputPng) ]

        for p in tab: 
            if p[0] in types: p[1](props)

    ##----------------------------------------

    def outputDot (self, props):
        fn = "%s.dot" % self.fileStem (props)
        self._dotfile = fn
        f = open (fn, "w")

        nodes = self._sorted_nodes[0:props.numNodes ()]
        node_set = set ([ n.id () for n in nodes ] )

        print >>f, "digraph ssp_profile_%d {" % self._serial

        mhs = 0
        for n in nodes:
            mhs = max (mhs, n.hitsSelf ())

        for n in nodes:
            hs = n.hitsSelf ()
            h = n.hits ()

            label = "%s\\n%s\\n%s" % \
                (csym_split (n.name ()), self.pct (h), self.pct (hs))

            params = [ ( 'label' , label ),
                       ( 'fillcolor', self.color (h)),
                       ( 'penwidth', self.penWidth (hs, mhs)),
                       ( 'shape', 'ellipse' ),
                       ( 'style', 'filled' ),
                       ( 'fontsize', str (props.fontSize ())) ]

            s = '\t "%d" %s;' % (n.id (), toDotParams (params))

            print >>f, s

        edges = self._sorted_edges
        n = 0

        for e in edges:
            er = e.caller ().id ()
            ee = e.callee ().id ()

            if er in node_set and ee in node_set:

                params = [ ('label', self.pct (e.hits ())), 
                           ('fontsize', str (props.fontSize ())) ]

                s = '\t "%d" -> "%d" %s ;' %  (er, ee, toDotParams (params))

                print >>f, s
                n += 1

                if n > props.numEdges ():
                    break

        print >>f, """\tlabel = "\\n%s";""" % \
            '\\n'.join (self.headerText (props))

        print >>f, "}"

    ##----------------------------------------

    def outputPs (self, props):
        fn = "%s.ps" % self.fileStem (props)
        cmd = [ DOT , "-Tps", "-o", fn, self._dotfile ]
        rc = subprocess.call (cmd)

    ##----------------------------------------

    def outputPng (self, props):
        fn = "%s.png" % self.fileStem (props)
        cmd = [ DOT , "-Tpng", "-o", fn, self._dotfile ]
        rc = subprocess.call (cmd)

    ##----------------------------------------

    def fileStem (self, props):
        return "%s-%d" % (props.stem (), self._serial)

    ##----------------------------------------

    def headerText (self, props):
        v =  [ "infile: %s" % props.filename (),
               "program: %s" % self._prog ,
               "begin: %s" % self._begin,
               "end: %s" % self._end ]
        return v

    ##----------------------------------------

    def outputText (self, props):
        fn = "%s.txt" % self.fileStem (props)
        f = open (fn, "w")

        print >>f, '\n'.join (self.headerText (props))
        print >>f

        self.dump (f)

##=======================================================================

class Parser:
    """Top level parser."""

    begin_rxx = re.compile ("\+{4} start report \+{4}")
    end_rxx = re.compile ("\-{4} end report \-{4}")

    line_rxx = re.compile ("^((?P<prefix>.*?)\s+)?" +
                           "(?P<prog>\w+):\s+" +
                           "\(SSP\)\s+" +
                           "(?P<meat>.*?)\s*$")

    ##----------------------------------------

    def eof (self):
        return self._stream.eof ()

    ##----------------------------------------

    def __init__ (self, s):
        self._stream = s
        
    ##----------------------------------------

    def getNextLine (self):
        for raw in self._stream:
            m = self.line_rxx.match (raw)
            if m is not None:
                return Line (**m.groupdict ())
        return None

    ##----------------------------------------

    def parseGraph (self, serial, props):
        lines = []
        go = True
        while not self.eof () and go:
            x = self.getNextLine ()
            if not x:
                go = False
            elif self.begin_rxx.match (x.meat ()):
                while not self.eof () and go:
                    x = self.getNextLine ()
                    if not x or self.end_rxx.match (x.meat ()):
                        go = False
                    else:
                        lines.append (x)
        ret = None
        if len(lines):
            ret = Graph (lines, serial, props)
        return ret 

    ##----------------------------------------

    def run (self, props):

        serial = 0

        while not self.eof ():
            graph = self.parseGraph (serial, props)
            serial += 1
            if graph:
                graph.output (props)

##=======================================================================

class Props:
    
    ##----------------------------------------

    def __init__ (self, argv):
        self._cmd = argv[0]
        self._num_edges = 500
        self._num_nodes = 50
        self._file = None
        self._jail = None
        self._types = OutputTypes.All ()
        self._inlining = False
        self._font_size = 20
        self._stem = "ssp"

        self.parse (argv)

    ##----------------------------------------

    def numNodes (self):
        return self._num_nodes

    ##----------------------------------------
    
    def stem (self):
        return self._stem

    ##----------------------------------------

    def fontSize (self):
        return self._font_size

    ##----------------------------------------

    def inlining (self):
        return self._inlining

    ##----------------------------------------

    def numEdges (self):
        return self._num_edges

    ##----------------------------------------

    def file (self):
        return self._file

    ##----------------------------------------

    def filename (self):
        return self._filename

    ##----------------------------------------

    def jail (self):
        return self._jail

    ##----------------------------------------

    def outputTypes (self):
        return self._types

    ##----------------------------------------

    def parse (self, argv):
        short_opts = "n:e:ht:j:if:s:"
        long_opts = [ "num-nodes=",
                      "num-edges=",
                      "jail=",
                      "type=",
                      "help",
                      "inlining",
                      "font-size=",
                      "stem=" ]

        types = []
        try:
            opts, args = getopt.getopt (sys.argv[1:], short_opts, long_opts)
        except getopt.GetoptError:
            self.usage ()

        for o, a in opts:

            if o in ("-s", "--stem"):
                self._stem = a

            elif o in ("-f", "--font-size"):
                try:
                    self._font_size = int (a)
                except ValueError, e:
                    emsg ("Bad integer argument for -s supplied: %s" % a)
                    raise ProfileError, "bad argument"

            elif o in ("-i", "--inlining"):
                self._inlining = True

            elif o in ("-n", "--num-nodes"):
                try:
                    self._num_nodes = int (a)
                except ValueError, e:
                    emsg ("Bad integer argument for -n supplied: %s" % a)
                    raise ProfileError, "bad argument"

            elif o in ("-e", "--num-edges"):
                try:
                    self._num_edges = int (a)
                except ValueError, e:
                    emsg ("Bad integer argument for -e supplied: %s" % a)
                    raise ProfileError, "bad argument"

            elif o in ("-h", "--help"):
                self.usage (ok = True)

            elif o in ("-j", "--jail"):
                self._jail = a

            elif o in ("-t", "--type"):
                try:
                    t = OutputTypes.from_str (a)
                except ValueError, e:
                    emsg ("Bad output type: %s" % a)
                    raise ProfileError, "bad argument"
                types.append (t)
                if t == OutputTypes.PS or t == OutputTypes.PNG:
                    types.append (OutputTypes.DOT)

            else:
                self.usage (err = "unknown argument: %s" % o)

        if len(types) > 0:
            self._types = set (types)

        if len(args) == 1:
            fn = args[0]
            f = open (fn, "r")
            if f is None:
                emsg ("Cannot open file: %s" % fn)
                raise ProfileError, "I/O error"
            self._file = f
            self._filename = fn
        elif len(args) == 0:
            self._file = sys.stdin
            self._filename = "<stdin>"
        else:
            self.usage (err = "too many arguments")

    ##----------------------------------------

    def usage (self, err = "command line error", ok = False):
        print """
%s [-ne] [<file>]

  A script to read in Simple SFS embedded Profiler (SSP) 
  output and make sense of it.  It reads input from stdin by default,
  or from a file if given.

  Options are:
    -s <str>, --stem=<str>
         The stem to use ('ssp' by default)
    -f <int>, --font-size=<int>
         Font size of all text in the graph
    -i, --inlining
         Try to de-inline (if addr2line supports -i)
    -n <num>, --num-nodes=<num>
         Number of nodes to show in the output (default = 50)
    -e <num>, --num-edges=<num>
         Number of edges to show in the output (default = 500)
    -j <jail>, --jail=<jail>
         Specify a jail directory (default = /)
    -t <type> --type=<type>
         Output type, one or more of { 'txt', 'dot', 'png', 'eps' }
""" % self._cmd.split ('/')[-1]

        if ok:
            sys.exit (0)

        raise ProfileError, err

    ##----------------------------------------


##=======================================================================

def main (argv):

    props = Props (argv)
    parser = Parser (Stream (props.file ()))
    parser.run (props)

##=======================================================================

try:
    main (sys.argv)
except ProfileError, e:
    emsg ("profile exitting uncleanly (%s)" % e, "++ ", " ++")
    sys.exit (2)

##=======================================================================
