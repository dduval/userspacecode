#!/usr/bin/python

#  Copyright Dominic Duval <dd@dd.qc.ca> according to the terms
# of the GNU Public License.

import rpm
import sys

have_miscutils = 0
try:
    from rpmUtils.miscutils import stringToVersion
    have_miscutils = 1
except:
    pass


def rpmvercmp(rpm1, rpm2):
    (e1, v1, r1) = stringToVersion(rpm1)
    (e2, v2, r2) = stringToVersion(rpm2)
    if e1 is not None: e1 = str(e1)
    if e2 is not None: e2 = str(e2) 
    rc = rpm.labelCompare((e1, v1, r1), (e2, v2, r2))     
    return rc

def usage():
    print """
sortrpmlistbyversion.py <file_containing_package_list>
"""

def main():
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help', '-help', '--usage']:
        usage()
        sys.exit(0)
    elif len(sys.argv) == 2:
        file = open(sys.argv[1], "r")
        lines = file.readlines()
        lines.sort(rpmvercmp)
        for line in lines:
            sys.stdout.write( line )
        file.close()
        sys.exit(0)


if __name__ == "__main__":
    main()

