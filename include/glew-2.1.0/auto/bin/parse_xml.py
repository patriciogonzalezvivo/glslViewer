#!/usr/bin/python

import re
import sys
from xml.dom.minidom import parse, Node

def findChildren(node, path):
    result = []
    if len(path)==1:
        for i in node.childNodes:
            if i.nodeType==Node.ELEMENT_NODE:
                if i.tagName==path[0]:
                    result.append(i)
    else:
        for i in node.childNodes:
            if i.nodeType==Node.ELEMENT_NODE:
                if i.tagName==path[0]:
                    result.extend(findChildren(i, path[1:]))
    return result

def findData(node, path):
    return [ i.firstChild.data for i in findChildren(node, path) ]

def findParams(node):
    n = findData(node, ['name'])[0]
    t = ''
    for i in node.childNodes:
        if i.nodeType==Node.TEXT_NODE:
            t += i.data
        if i.nodeType==Node.ELEMENT_NODE and i.tagName=='ptype':
            t += i.firstChild.data
    return ( t, n)

def findEnums(dom):
    ret = {}
    for i in findChildren(dom, [ 'registry', 'enums', 'enum' ]):
      n = i.getAttribute('name')
      v = i.getAttribute('value')
      ret[n] = v
    return ret

def findCommands(dom):
    ret = {}
    for i in findChildren(dom, [ 'registry', 'commands', 'command' ]):
      r,n = findParams(findChildren(i, ['proto'])[0])
      p = [ findParams(j) for j in findChildren(i, ['param'])]
      ret[n] = (r, p)
    return ret

def findFeatures(dom):
    ret = {}
    for i in findChildren(dom, [ 'registry', 'feature' ]):
        n = i.getAttribute('name')
        e = []
        c = []
        for j in findChildren(i, [ 'require', 'enum' ]):
            e.append(j.getAttribute("name"))
        for j in findChildren(i, [ 'require', 'command' ]):
            c.append(j.getAttribute("name"))
        ret[n] = (e,c)
    return ret

def findExtensions(dom):
    ret = {}
    for i in findChildren(dom, [ 'registry', 'extensions', 'extension' ]):
        n = i.getAttribute('name')
        e = []
        c = []
        for j in findChildren(i, [ 'require', 'enum' ]):
            e.append(j.getAttribute("name"))
        for j in findChildren(i, [ 'require', 'command' ]):
            c.append(j.getAttribute("name"))
        ret[n] = (e,c)
    return ret

def findApi(dom, name):
    enums      = findEnums(dom)
    commands   = findCommands(dom)
    features   = findFeatures(dom)
    extensions = findExtensions(dom)
    return (enums, commands, features, extensions)

def writeExtension(f, name, extension, enums, commands):
    f.write('%s\n'%name)
    f.write('%s\n'%'https://www.khronos.org/registry/egl/specs/eglspec.1.5.pdf')
    if name.find('_VERSION_')==-1:
        f.write('%s\n'%name)
    else:
        f.write('\n')
    f.write('\n')
    enums = [ (j, enums[j]) for j in extension[0] ]
    for e in sorted(enums, key=lambda i: i[1]):
        f.write('\t%s %s\n'%(e[0], e[1]))
    commands = [ (j, commands[j]) for j in extension[1] ]
    for c in sorted(commands):
        params = ', '.join( [ '%s %s'%(j[0], j[1]) for j in c[1][1] ] )
        if len(params)==0:
            params = ' void '
        f.write('\t%s %s (%s)\n'%(c[1][0], c[0], params))

if __name__ == '__main__':

  from optparse import OptionParser
  import os

  parser = OptionParser('usage: %prog [options] [XML specs...]')
  parser.add_option("--core", dest="core", help="location for core outputs", default='')
  parser.add_option("--extensions", dest="extensions", help="location for extensions outputs", default='')

  (options, args) = parser.parse_args()

  for i in args:

    dom = parse(i)
    api = findApi(dom, 'egl')

    print('Found {} enums, {} commands, {} features and {} extensions.'.format(
        len(api[0]), len(api[1]), len(api[2]), len(api[3])))

    if len(options.core):
        for i in api[2].keys():
            f = open('%s/%s'%(options.core, i), 'w')
            writeExtension(f, i, api[2][i], api[0], api[1])
            f.close()

    if len(options.extensions):
        for i in api[3].keys():
            f = open('%s/%s'%(options.extensions, i), 'w')
            writeExtension(f, i, api[3][i], api[0], api[1])
            f.close()

