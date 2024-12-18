project(
  'raylib_orthographic', 
  'cpp', 
  version: '0.2.0', 
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
    'buildtype=debug',
    'optimization=0'
  ],
)

# Compiler options based on system type
if host_machine.system() == 'linux' or host_machine.system() == 'darwin' # macOS
    add_project_arguments([
        '-Wno-unused-parameter',
        '-Wno-unused-variable',
        '-Wno-unused-function',
        '-Wno-unused-const-variable',
        '-Wno-unused-lambda-capture',
        '-Wno-unused-local-typedef',
        '-Wno-unused-value',
        '-Wno-missing-field-initializers',
        '-Wno-unused-but-set-variable'
    ], language: 'cpp')
elif host_machine.system() == 'windows'
    add_project_arguments(['/W4'], language: 'cpp')
endif

# Find the system-installed Raylib library
raylib_dep = dependency('raylib', required: true, version: '>=4.5.0')

# Include directories
inc_dir = include_directories('includes')

# Define the executable
exe = executable(
    'raylib_orthographic', 
    'src/main.cpp',
    dependencies: raylib_dep,
    include_directories: inc_dir,
    install: false,
)

# Install directories and resources

# Create a resources directory in the build directory
resources_dir = custom_target(
  'resources_dir',
  output: 'resources',
  command: ['mkdir', '-p', meson.build_root() / './resources'],
  build_by_default: true
)

fonts_copy = custom_target(
  'copy_fonts',
  output: 'fonts',
  command: ['cp', '-r', meson.source_root() / './resources/fonts', meson.build_root() / './resources'],
  build_by_default: true
)

fonts_copy = custom_target(
  'copy_images',
  output: 'images',
  command: ['cp', '-r', meson.source_root() / './resources/images', meson.build_root() / './resources'],
  build_by_default: true
)


