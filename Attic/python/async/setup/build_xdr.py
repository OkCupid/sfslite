"""sfs.setup.build_xdr

Implements the Distuitils 'build_xdr' command, to run the SFS rpcc
compiler on input .x files to output buildable .C and .h files."""

__revision__ = "$Id$"

import os, string
from types import *
from distutils.core import Command
from distutils.errors import *
from distutils.sysconfig import customize_compiler
from distutils import log

class build_xdr (Command):

    description = "copmile .x files into .C/.h files"

    user_options = [
        ('build-xdr', 'b',
         "directory to build .C/.h sources to"),
        ('force', 'f',
         "forcibly build everything (ignore file timestamps)"),
        ('rpcc', 'r',
         "specify the rpcc path"),
        ]

    boolean_options = ['force' ]

    def initialize_options (self):
        self.build_xdr = None
        self.force = False
        self.rpcc = None

    def finalize_options (self):
        self.set_undefined_options ('build',
                                    ('build_xdr', 'build_xdr'),
                                    ('force', 'force'),
                                    ('rpcc', 'rpcc'));

        self.rpcc = self.find_rpcc ()
