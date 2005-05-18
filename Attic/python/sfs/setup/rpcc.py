"""sfs.setup.rpcc

Implements an RPCC compiler abstraction for building Python
RPC/XDR modules for use with SFS."""

__revision__ = "$Id$"

import os, string
import os.path
from types import *
from distutils.core import Command
from distutils.errors import *
from distutils.sysconfig import customize_compiler
from distutils import log

import sfs.setup.local

class rpcc:
    """Wrapper class around the SFS C compiler."""

    HEADER=1
    CFILE=2

    def __init__ (self,
                  loc=None,
                  dry_run=0,
                  verbose=0):
        self.dry_run = dry_run
        self.verbose = verbose
        self.rpcc = find_rpcc (self, loc)

    def spawn (self, cmd):
        spawn (cmd, dry_run=self.dry_run, verbose=self.verbose)
    
    def find_rpcc (self, loc):
        executable = 'rpcc'
        
        if loc is not None and os.path.isfile (loc):
            return loc
                
        paths = string.split (os.environ['PATH'], os.pathsep)
        for p in paths + sfs.setup.local.lib:
            f = os.path.join (p, executable)
            if os.path.isfile (f):
                return loc
        raise DistutilsSetupError, \
              "cannot find an rpcc compiler"
    
    # find_rpcc ()

    def compile_all (self, sources, output_dir):
        if self.loc is None:
            raise DistutilsSetupError, "no rpcc compiler found"
        for s in sources:
            base, ext = os.path.splitext (s)
            if ext != '.x':
                raise DistutilsSetupError, \
                      "file without .x extension: '%s'" % s
            if output_dir:
                dir, fl = os.path.split (s)
                outfile = os.path.join (output_dir, fl)
            else:
                outfile = s

            self.compile_hdr (s, outfile)
            self.compile_cfile (s, outfile)

    # compile_all ()

    def compile_hdr (self, in, out):
        self.compile (in, out HEADER)

    def compile_cfile (self, in, out):
        self.compile (in, out, CFILE)

    def compile (self, in, out, mode):
        if mode == HEADER:
            arg = "-pyh"
        elif mode == CFILE:
            arg = "-pyc"
        else:
            raise DistutilsSetupError, \
                  "bad compile mode given to rpcc.compile"
        cmd = ( self.rpcc, arg, '-o', out, in)
        self.spawn (cmd)

    # compile ()
