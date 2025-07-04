# Copyright 2011 The LibYuv Project Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS. All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [
    'libyuv.gypi',
  ],
  # Make sure that if we are being compiled to an xcodeproj, nothing tries to
  # include a .pch.
  'xcode_settings': {
    'GCC_PREFIX_HEADER': '',
    'GCC_PRECOMPILE_PREFIX_HEADER': 'NO',
  },
  'variables': {
    'use_system_libjpeg%': 0,
    # Can be enabled if your jpeg has GYP support.
    'libyuv_disable_jpeg%': 1,
    # 'chromium_code' treats libyuv as internal and increases warning level.
    'chromium_code': 1,
    # clang compiler default variable usable by other apps that include libyuv.
    'clang%': 0,
    # Link-Time Optimizations.
    'use_lto%': 0,
    # Enable LASX on LoongArch by default.
    "loong_lasx%": 1,
    # Enable LSX on LoongArch by default. Has no effect if loong_lasx is
    # enabled because LASX implies LSX according to the architecture specs.
    "loong_lsx%": 1,
    'mips_msa%': 0,  # Default to msa off.
    'build_neon': 0,
    "build_lasx": 0,
    "build_lsx": 0,
    'build_msa': 0,

    'conditions': [
       ['(target_arch == "armv7" or target_arch == "armv7s" or \
       (target_arch == "arm" and arm_version >= 7) or target_arch == "arm64")\
       and (arm_neon == 1 or arm_neon_optional == 1)', {
         'build_neon': 1,
       }],
       ['(target_arch == "loong64") and (loong_lasx == 1)', {
         "build_lasx": 1,
         "build_lsx": 1,  # LASX implies LSX.
       }],
       ['(target_arch == "loong64") and (loong_lsx == 1)', {
         "build_lsx": 1,
       }],
       ['(target_arch == "mipsel" or target_arch == "mips64el")\
       and (mips_msa == 1)',
       {
         'build_msa': 1,
       }],
    ],
  },

  'targets': [
    {
      'target_name': 'libyuv',
      # Change type to 'shared_library' to build .so or .dll files.
      'type': 'static_library',
      'variables': {
        'optimize': 'max',  # enable O2 and ltcg.
      },
      # Allows libyuv.a redistributable library without external dependencies.
      # 'standalone_static_library': 1,
      'conditions': [
       # Disable -Wunused-parameter
        ['clang == 1', {
          'cflags': [
            '-Wno-unused-parameter',
         ],
        }],
        ["build_lasx != 0", {
          "cflags": ["-mlasx"],
        }, {  # build_lasx == 0
          "cflags": ["-mno-lasx"],
        }],
        ["build_lsx != 0", {
          "cflags": ["-mlsx"],
        }, {  # build_lsx == 0
          "cflags": ["-mno-lsx"],
        }],
        ['build_neon != 0', {
          'defines': [
            'LIBYUV_NEON',
          ],
          'cflags!': [
            '-mfpu=vfp',
            '-mfpu=vfpv3',
            '-mfpu=vfpv3-d16',
            # '-mthumb',  # arm32 not thumb
          ],
          'cflags_mozilla!': [
            '<@(moz_neon_cflags_block_list)',
          ],
          'conditions': [
            # Disable LTO in libyuv_neon target due to gcc 4.9 compiler bug.
            ['clang == 0 and use_lto == 1', {
              'cflags!': [
                '-flto',
                '-ffat-lto-objects',
              ],
            }],
          ],
        }],
        ['build_msa != 0', {
          'defines': [
            'LIBYUV_MSA',
          ],
        }],
        ['build_with_mozilla == 1', {
          'defines': [
            'HAVE_JPEG'
          ],
          'cflags_mozilla': [
            '$(MOZ_JPEG_CFLAGS)',
          ],
        }],
        ['OS != "ios" and libyuv_disable_jpeg != 1 and build_with_mozilla != 1', {
          'defines': [
            'HAVE_JPEG'
          ],
          'conditions': [
            # Caveat system jpeg support may not support motion jpeg
            [ 'use_system_libjpeg == 1', {
              'dependencies': [
                 '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
              ],
            }, {
              'dependencies': [
                 '<(DEPTH)/third_party/libjpeg_turbo/libjpeg.gyp:libjpeg',
              ],
            }],
            [ 'use_system_libjpeg == 1', {
              'link_settings': {
                'libraries': [
                  '-ljpeg',
                ],
              }
            }],
          ],
        }],
      ], #conditions
      'defines': [
        'LIBYUV_DISABLE_SME',
        # Enable the following 3 macros to turn off assembly for specified CPU.
        # 'LIBYUV_DISABLE_X86',
        # 'LIBYUV_DISABLE_NEON',
        # 'LIBYUV_DISABLE_DSPR2',
        # Enable the following macro to build libyuv as a shared library (dll).
        # 'LIBYUV_USING_SHARED_LIBRARY',
        # TODO(fbarchard): Make these into gyp defines.
      ],
      'include_dirs': [
        'include',
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
          '.',
        ],
        'conditions': [
          ['OS == "android" and target_arch == "arm64"', {
            'ldflags': [
              '-Wl,--dynamic-linker,/system/bin/linker64',
            ],
          }],
          ['OS == "android" and target_arch != "arm64"', {
            'ldflags': [
              '-Wl,--dynamic-linker,/system/bin/linker',
            ],
          }],
          ['target_arch == "armv7" or target_arch == "arm64" and moz_have_arm_i8mm_and_dot_prod == 1 and build_with_mozilla == 1', {
            'dependencies': [
                 ':libyuv_neon',
            ],
          }],
          ['target_arch == "arm64" and moz_have_arm_sve2 == 1 and build_with_mozilla == 1', {
            'dependencies': [
                 ':libyuv_sve',
            ],
            'defines' :[
              'LIBYUV_SVE',
            ]
          }],
          ['target_arch == "arm64" and moz_have_arm_sve2 == 1 and build_with_mozilla == 1', {
            'dependencies': [
                 ':libyuv_sve',
            ],
            'defines' :[
              'LIBYUV_SVE',
            ]
          }],
        ], #conditions
      },
      'sources': [
        '<@(libyuv_sources)',
      ],
    },
    {
      'target_name': 'libyuv_neon',
      'type': 'static_library',
      'variables': {
        'optimize': 'max',  # enable O2 and ltcg.
      },
      'conditions': [
        ['target_arch == "arm64" and moz_have_arm_i8mm_and_dot_prod == 1 and build_with_mozilla == 1', {
          'cflags_mozilla': [
            '-march=armv8.2-a+dotprod+i8mm',
          ],
        }],
        # arm64 does not need -mfpu=neon option as neon is not optional
        ['target_arch != "arm64"', {
          'cflags': [
            '-mfpu=neon',
            # '-marm',  # arm32 not thumb
          ],
          'cflags_mozilla': [
            '-mfpu=neon',
          ],
        }],
        ['build_neon != 0', {
          'cflags_mozilla!': [
            '<@(moz_neon_cflags_block_list)',
          ],
          'sources': [
            '<@(libyuv_neon_sources)',
          ],
        }],
     ], #conditions
      'include_dirs': [
        'include',
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
          '.',
        ], #conditions
      },
    },
    {
      'target_name': 'libyuv_sve',
      'type': 'static_library',
      'variables': {
        'optimize': 'max',  # enable O2 and ltcg.
      },
      'conditions': [
        ['target_arch == "arm64" and moz_have_arm_sve2 == 1 and build_with_mozilla == 1', {
          'cflags_mozilla!': [
            '<@(moz_neon_cflags_block_list)',
          ],
          'cflags_mozilla': [
            '-march=armv9-a+dotprod+sve2+i8mm',
          ],
          'sources': [
            '<@(libyuv_sve_sources)',
          ],
        }],
     ], #conditions
      'include_dirs': [
        'include',
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
          '.',
        ], #conditions
      },
    },
  ], # targets.
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
