{
    'targets': [{
        'target_name': 'vips',
        'sources': [
            'src/myconvert.cc',
            'src/node-vips.cc',
            'src/transform.cc',
            'src/transform.h'
        ], 
        'libraries': ['-lglib2', '-lvips', '-lexiv2']
    }]
}
