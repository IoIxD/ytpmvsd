#!/usr/bin/env python3

libraries = ["avcodec","avdevice","avfilter","avformat","avutil","postproc","swscale","swresample"]
lib_dir = '/usr/lib64'

extra_objects = ['{}/lib{}.so'.format(lib_dir, l) for l in libraries]

from setuptools import setup, Extension

setup(
	name = "proberffi",
	version = "1.0",
	ext_modules = [
        Extension(
            "proberffi", 
            ["bind.cpp", "prober.cpp", "stream.cpp"], 
            	# libraries=libraries,
                #  library_dirs=library_dirs,
                #  include_dirs=include_dirs,
                 extra_objects=extra_objects
            )],
    include_dirs=["/usr/include/ffmpeg"],
)
