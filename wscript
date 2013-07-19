## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('igmpx', ['internet', 'wifi'])
    module.includes = '.'
    module.source = [
        'model/igmpx-packet.cc',
        'model/igmpx-routing.cc',
        'helper/igmpx-helper.cc',
        ]
        
    headers = bld.new_task_gen(features=['ns3header'])
    headers.module = 'igmpx'
    headers.source = [
        'model/igmpx-packet.h',        
        'model/igmpx-routing.h',
        'helper/igmpx-helper.h',
    ]
               
    module_test = bld.create_ns3_module_test_library('igmpx')
    module_test.source = [
          'test/igmpx-header-test-suite.cc',
          ]
    
    if bld.env['ENABLE_EXAMPLES']:
        bld.add_subdirs('examples')
        
#    bld.ns3_python_bindings()
