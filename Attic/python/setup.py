
metadata = {
    'name': 'pyasync',
    'version': '0.1',
    'description': "Python wrappers for libasync",
    'author': "Max Krohn",
    'author_email': "max@maxk.org",
    'license': "GPL",
    'platforms': "ALL",
    'url': "http://sourceforge.net/projects/mysql-python",
    'download_url': "http://prdownloads.sourceforge.net/mysql-python/" \
                    "MySQL-python-%s.tar.gz" % version,
    'classifiers': [ c for c in classifiers.split('\n') if c ],
    'py_modules': [
        "_mysql_exceptions",
        "MySQLdb.converters",
        "MySQLdb.connections",
        "MySQLdb.cursors",
        "MySQLdb.sets",
        "MySQLdb.times",
        "MySQLdb.stringtimes",
        "MySQLdb.mxdatetimes",
        "MySQLdb.pytimes",
        "MySQLdb.constants.CR",
        "MySQLdb.constants.FIELD_TYPE",
        "MySQLdb.constants.ER",
        "MySQLdb.constants.FLAG",
        "MySQLdb.constants.REFRESH",
        "MySQLdb.constants.CLIENT",
        ],
    'ext_modules': [
        Extension(
            name='async/core',
            sources=['async.core'],
            include_dirs=['']
            library_dirs=library_dirs,
            libraries=libraries,
            extra_compile_args=extra_compile_args,
            extra_objects=extra_objects,
            ),
        ],
    }

setup(**metadata)
