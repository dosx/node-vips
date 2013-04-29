{
    'targets': [{
        'target_name': 'vips',
        'sources': [
            'src/node-vips.cc',
            'src/transform.cc'
        ], 
        'libraries': [
            'pkg-config --cflags glib-2.0', 
            'pkg-config --cflags vips', 
            'pkg-config --cflags exiv2'
        ],
        'include_dirs': [
            '/usr/include/glib-2.0', 
            '/usr/lib/glib-2.0/include',
            '/usr/lib/x86_64-linux-gnu/glib-2.0/include'
        ]
    }]
}
