{
    'targets': [{
        'target_name': 'vips',
        'sources': [
            'src/node-vips.cc',
            'src/transform.cc'
        ],
        'conditions': [
          ['OS=="mac"', {
            'libraries': [
                '<!@(PKG_CONFIG_PATH=/usr/local/Library/ENV/pkgconfig/10.8 pkg-config --libs glib-2.0 vips exiv2)',
            ],
            'include_dirs': [
              '/usr/local/include/glib-2.0',
              '/usr/local/include/vips',
              '/usr/local/include/exiv2',
              '/usr/local/lib/glib-2.0/include'
            ]
          }, {
            'libraries': [
                '<!@(PKG_CONFIG_PATH="/usr/local/lib/pkgconfig" pkg-config --libs glib-2.0 vips exiv2)'
            ],
            'include_dirs': [
                '/usr/include/glib-2.0',
                '/usr/lib/glib-2.0/include',
                '/usr/lib/x86_64-linux-gnu/glib-2.0/include'
            ],
          }]
        ],
        'cflags': [ '-fexceptions' ],
        'cflags_cc': [ '-fexceptions' ]
    }]
}
