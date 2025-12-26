import os
import subprocess

def generate_resource_cpp(path_to_raw_file: str, resource_name: str, resource_type: str):
    with open(path_to_raw_file, 'rb') as raw_file:
        raw_data = raw_file.read()
        with open(f'./Intermediate/Resources/{resource_name}.h', 'w') as h_file:
            h_file.write(f'''
#pragma once
#include "common/Resource.h"
namespace Resources {{
extern Resource {resource_name.upper()};
}}
''')
        with open(f'./Intermediate/Resources/{resource_name}.cpp', 'w') as cpp_file:
            cpp_file.write(f'''
#include "{resource_name}.h"
namespace Resources {{
Resource {resource_name.upper()} = Resource(
    ResourceType::{resource_type},
    {len(raw_data)},
    new byte[] {{ {', '.join(f'0x{b:02X}' for b in raw_data)} }}
);
}}
''')

def compile_resources(dir: str):
    for dirpath, dirnames, filenames in os.walk(dir):
        for filename in filenames:
            if filename.endswith('.glsl'):
                subprocess.run(['glslangValidator', '-V', f'{dirpath}/{filename}', '-o', f'./Intermediate/ShaderDump/{filename}.spv'])
                generate_resource_cpp(f'./Intermediate/ShaderDump/{filename}.spv', filename.split('.')[0], 'Shader')

os.makedirs('./Intermediate/ShaderDump', exist_ok=True)
os.makedirs('./Intermediate/Resources', exist_ok=True)
compile_resources('./ShaderCode')