## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('video-push', ['internet', 'network', 'config-store', 'tools', 'pimdm', 'wifi', 'applications', 'csma', 'stats'])
    module.includes = '.'
    module.source = [
        'model/chunk-packet.cc',
        'model/chunk-buffer.cc',		
        'model/neighbor-set.cc',
        'model/video-push.cc',       
        'helper/video-helper.cc',
        ]

    headers = bld.new_task_gen(features=['ns3header'])
    headers.module = 'video-push'
    headers.source = [        
        'model/chunk-video.h',
        'model/chunk-packet.h',
        'model/chunk-buffer.h',
        'model/neighbor.h',
        'model/neighbor-set.h',
        'model/video-push.h',        
        'helper/video-helper.h',
    ]

    module_test = bld.create_ns3_module_test_library('video-push')
    module_test.source = [
          'test/chunk-header-test-suite.cc',
          'test/chunk-buffer-test-suite.cc'
          ]
    
    if bld.env['ENABLE_EXAMPLES']:
        bld.add_subdirs('examples')
        
#        bld.ns3_python_bindings()
