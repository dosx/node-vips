{
    'targets': [{
        'target_name': 'vips',
        'sources': [
            'src/node-vips.cc',
            'src/transform.cc'
        ], 
        'libraries': [
            '<!@(PKG_CONFIG_PATH="/usr/local/lib/pkgconfig" pkg-config --cflags glib-2.0)', 
            '<!@(PKG_CONFIG_PATH="/usr/local/lib/pkgconfig" pkg-config --cflags vips)', 
            '<!@(PKG_CONFIG_PATH="/usr/local/lib/pkgconfig" pkg-config --cflags exiv2)'
        ],
        'include_dirs': [
            '/usr/include/glib-2.0', 
            '/usr/lib/glib-2.0/include',
            '/usr/lib/x86_64-linux-gnu/glib-2.0/include'
        ],
        'link_settings': { 
            'ldflags': [ '-lglib-2.0', '-lvips', '-lexiv2' ], 
        }, 
        'cflags': [ '-fexceptions' ],
        'cflags_cc': [ '-fexceptions' ]
    }]
}
