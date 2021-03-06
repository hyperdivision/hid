{
  'variables': {
    'target_arch%': '<!(node preinstall.js --print-arch)>'
  },
  'targets': [
    {
      'target_name': 'hidapi',
      'include_dirs' : [
        '<!(node -e \"require(\'napi-macros\')\")',
        'hidapi'
      ],
      'sources': [
        'index.c'
      ],
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-g',
          '-O3',
        ]
      },
      'cflags': [
        '-g',
        '-O3',
      ],
      'libraries': [
        '<!(node preinstall.js --print-lib)'
      ],
      'conditions': [
        ['OS != "mac" and OS != "win"', {
          'link_settings': {
            'libraries': [ "-Wl,-rpath=\\$$ORIGIN"]
          }
        }],
        [ 'OS=="mac"', {
          'LDFLAGS': [
            '-framework IOKit',
            '-framework CoreFoundation'
          ],
          'xcode_settings': {
            'CLANG_CXX_LIBRARY': 'libc++',
            'OTHER_LDFLAGS': [
              '-framework IOKit',
              '-framework CoreFoundation'
            ],
          }
        }],
        [ 'OS=="linux"', {
          'libraries': [
            '-ludev',
            '-lusb-1.0'
          ]
        }]
      ],
    }
  ]
}
