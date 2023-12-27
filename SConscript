from building import *

cwd  = GetCurrentDir()
objs = []
path = [cwd]
path += [cwd + '/inc']

src  = Glob('src/*.c')
src  += Glob('adapter/decompress/*.c')
src  += Glob('adapter/digest/*.c')


group = DefineGroup('uota', src, depend = [''], CPPPATH = path)

objs = group

list = os.listdir(cwd)
for item in list:
    if os.path.isfile(os.path.join(cwd, item, 'SConscript')):
        objs = objs + SConscript(os.path.join(item, 'SConscript'))

Return('objs')
