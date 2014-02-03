#! /usr/bin/env python

""" Sync YAML config file for distro configuration 

distroconfig.py <DIR>

"""
   
import yaml
import sys
import os

# Print usage message and exit
def usage():
    sys.stdout = sys.stderr
    print __doc__
    sys.exit(2)

# Dump directory entries to distros.yml
def dirtoyaml(dir):

    dirlist=[]
    dircontent = os.listdir(dir)
    if verbose > 1: print 'Directory content:' , dircontent

    for entry in dircontent:
        if os.path.isdir(entry):
            dirlist.append(entry)

    yamlfile=open('distros.yml', 'w')
    yaml.dump(dirlist, yamlfile)
    return len(dirlist)     

def loaddistros(dir):
    dirlist=[]
    yamlfile=open(dir + '/distros.yml', 'r')
    dirlist= yaml.load(yamlfile)
    return dirlist


def main(args):
    if verbose > 1 : print "main arguments:" , args
    if args[0] == 'load':
         print loaddistros(args[1])
    elif args[0] == 'dump':
        dirtoyaml(args[1])

if __name__ == "__main__":
    verbose=1
    main(sys.argv[1:])
