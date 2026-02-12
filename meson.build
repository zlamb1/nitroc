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

lexer_fail_tests = []
lexer_pass_tests = [ 'tokens' ]

foreach t : lexer_fail_tests
    test('lexer \'' + t + '\' test (fail)', exe, args : files('tests/lexer/fail/' / (t + '.nitro')))
endforeach

foreach t : lexer_pass_tests
    test('lexer \'' + t + '\' test (pass)', exe, args : files('tests/lexer/pass/' / (t + '.nitro')))
endforeach
