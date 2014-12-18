src_files =  Glob('*.cpp')
include_path = []
lib_path = []
libs = []
cc_flags = ['-g']

object_files = []
for node in src_files:
    sf = str(node)
    of = ''
    if sf.endswith('.c') == True:
        of = sf[ : sf.find('.c')] + '.o'
    elif sf.endswith('.cxx') == True:
        of = sf[ : sf.find('.cxx')] + '.o'
    else:
        of = sf[ : sf.find('.cpp')] + '.o'
    object_files += Object(target = 'objs/%s' % of, source = sf, CPPPATH = include_path, CCFLAGS = cc_flags)

Program(target = './objs/main', source = object_files, CPPPATH = include_path, LIBS = libs, LIBPATH = lib_path, CCFLAGS = cc_flags)
