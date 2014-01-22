#! /usr/bin/env python

""" Mirror a remote ftp dir into a local directory tree (files only)

ftpsync.py -s <ftpsite> -f <ftpdir> -l <localdir> -p <pattern>

Example:

    ./ftpsync.py -s ftp.redhat.com -f /pub/redhat/linux/updates/enterprise/4AS/en/os/SRPMS/ -l /tmp/ -p kernel-utils*
"""
   
import os
import sys
import getopt
import ftplib
from fnmatch import fnmatch

# Print usage message and exit
def usage():
    sys.stdout = sys.stderr
    print __doc__
    sys.exit(2)

def ftpsync(ftphost,remotedir,localdir,pattern):
    verbose=2
    login='anonymous'
    passwd='lxr@src3.org'
    account=''
    host='ftp.redhat.com'

    
    f = ftplib.FTP()
    if verbose > 1: print "Connecting to '%s'..." % host
    f.connect(host)
    if verbose > 1:
        print 'Logging in as %r...' % (login or 'anonymous')
    f.login(login, passwd, account)
    pwd = f.pwd()
    f.cwd(remotedir)
    pwd = f.pwd()
    if verbose > 1: print 'PWD =', repr(pwd)

    if localdir and not os.path.isdir(localdir):
        if verbose > 1: print 'Creating local directory', repr(localdir)
        try:
            makedir(localdir)
        except os.error, msg: 
            print "Failed to establish local directory", repr(localdir)
            return

    listing = []
    f.retrlines('LIST', listing.append)

    for line in listing:
        if verbose > 3: print 'processing %s' % line
        words = line.split(None, 8)
        if len(words) < 6:
            if verbose > 1: print 'Skipping short line'
            continue
        filename = words[-1].lstrip()
        infostuff = words[-5:-1]
        mode = words[0]

        # See if the file matches our pattern
        skip = 0
        if verbose > 3: print "%s %s" % (filename, pattern)
        if not fnmatch(filename, pattern):
            skip = 1
        if skip:
            continue
        if verbose > 1: print 'Match for %s' % filename
        if mode[0] == 'd':
            continue
        if mode[0] == 'l':
            continue

        fullname = os.path.join(localdir, filename)
        #Create new file and retreive content
        if os.path.isfile(fullname):
            if verbose > 1: print '%s already exists' % fullname
            continue
        try:
            fp = open( fullname , 'wb')
        except IOError, msg:
            print "Can't create %r: %s" % (filename, msg)
            continue
        if verbose > 1:
            print 'Retrieving %r from %r ...' % (filename, pwd) 
        try:
            f.retrbinary('RETR ' + filename, fp.write, 8*1024)
        except ftplib.error_perm, msg:
            print msg
        fp.close()
    return len(listing)     


def main(args):
    ftpsite=''
    ftpdir=''
    localdir=''
    pattern=''
    try:
        opts, args = getopt.getopt(args,"hs:f:l:p:")
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            usage() 
            sys.exit()
        elif opt in ("-s"):
            ftpsite = arg
        elif opt in ("-f"):
            ftpdir = arg
        elif opt in ("-l"):
            localdir = arg
        elif opt in ("-p"):
            pattern = arg

    print 'FTP site is', ftpsite
    print 'FTP directory is', ftpdir
    print 'Local directory is', localdir
    print 'Search pattern is', pattern
    
    ftpsync(ftpsite, ftpdir, localdir, pattern)


if __name__ == "__main__":
    verbose=2
    main(sys.argv[1:])
