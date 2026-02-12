project(
    'nitroc', 'c', 
    default_options : [ 'warning_level=3', 'werror=true' ]
)

incdir = include_directories('include/')

c_args = []

if get_option('debug')
    c_args += '-DNITRO_DEBUG=1'
endif

exe = executable(
    'nitroc',
    files('src/arena.c', 'src/file.c', 'src/lex.c', 'src/main.c'),
    c_args : c_args,
    include_directories : incdir
)

test('lexer tokens test', exe, args : files('tests/lexer/tokens.nitro'))