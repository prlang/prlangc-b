project('prlangc-b', 'c')

glob = run_command('python', 'grabber.py', 
    check: true
)

compiler_src = glob.stdout().strip().split('\n')
compiler_inc = include_directories('.')

executable('prlangc',
    compiler_src,
    install: true,
    include_directories: compiler_inc
)