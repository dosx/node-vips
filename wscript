# -*- mode: python -*-

import Options, Utils, sys, os
from os import unlink, symlink, popen
from os.path import exists, islink

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.tool_options("compiler_cc")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("compiler_cc")
  conf.check_tool("node_addon")
  o = Options.options
  libpath  = ['/usr/lib', '/opt/local/lib']
  includes = ['/usr/include', '/usr/local/include', '/opt/local/include']
  conf.check_cfg(package='glib-2.0',
                 args='--cflags --libs',
                 uselib_store='Glib2')
  conf.check_cfg(package='vips-7.26',
                 args='--cflags --libs',
                 uselib_store='Vips')
  conf.check_cfg(package='exiv2',
                 args='--cflags --libs',
                 uselib_store='Exiv2')

def build(bld):
  node_vips = bld.new_task_gen("cxx", "shlib", "node_addon")
  #node_vips.cxxflags =  [ "-g" ]
  #node_vips.cflags = [ "" ]
  node_vips.target = "node-vips"
  node_vips.source = [ 'src/node-vips.cc', 'src/transform.cc' ]
  node_vips.includes = [ '/usr/include/glib-2.0',
                         '/usr/lib/x86_64-linux-gnu/glib-2.0/include',
                        ]
  node_vips.uselib = [ "Vips", "Exiv2", "Glib2" ]

def test(t):
  Utils.exec_command('make test')
