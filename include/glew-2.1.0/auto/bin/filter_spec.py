#!/usr/bin/python

import re

section = re.compile('^(Name|Name Strings?|Contact|Notice|Number|Dependencies|Overview|Issues|IP Status|Status|Version|New Procedures and Functions|New Tokens|Additions to .*|Changes to .*|Modifications to .*|Add new Section .*)\s*$')
token   = re.compile('^\s+(([A-Z0-9][A-Z0-9_x]*):?\s+((?:0x)?[0-9A-F]+)([^\?]*))?\s*$')
match   = [ 'Name', 'Name String', 'Contact', 'Notice', 'Name Strings', 'Version', 'Number', 'Dependencies', 'New Procedures and Functions', 'New Tokens']

if __name__ == '__main__':

  from optparse import OptionParser
  import os

  parser = OptionParser('usage: %prog [options] [SOURCES...]')
  (options, args) = parser.parse_args()

  for i in args:
      lines = open(i).readlines()
      f = open(i,'w')

      # Keep track of the current section as we iterate over the input
      current = ''
      for j in lines:

          # Detect the start of a new section
          m = section.match(j)
          if m:
              current = m.group(1).strip()
              if current in match:
                  print >>f, j,
              continue

          if current=='New Tokens':
              if token.match(j):
                  print >>f, j,
          elif current in match:
                  print >>f, j,

