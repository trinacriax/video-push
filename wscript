## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
	module = bld.create_ns3_module('video-push', ['internet', 'config-store', 'tools', 'point-to-point', 'wifi', 'mobility', 'csma'])
	module.includes = '.'
	module.source = [
		'model/chunk-buffer.cc',		
		'model/neighbor-set.cc',
		'model/video-push.cc',
		'helper/video-helper.cc',
		]

	headers = bld.new_task_gen('ns3header')	
	headers.module = 'video-push'
	headers.source = [
		'model/chunk-buffer.h',
		'model/chunk-video.h',
		'model/neighbor.h',
		'model/neighbor-set.h',
		'model/video-push.h',
		'helper/video-helper.h',
	]

	if bld.env['ENABLE_EXAMPLES']:
		bld.add_subdirs('examples')
		
		bld.ns3_python_bindings()