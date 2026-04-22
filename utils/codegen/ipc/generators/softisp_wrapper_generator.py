#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Simple generator that creates a C++ wrapper from a .mojom file using regex parsing.
# This avoids the complexity of the full mojom AST parser.

import argparse
import os
import re
import sys

def parse_mojom_methods(mojom_content):
    """Extract interface methods from mojom content using regex."""
    methods = []
    
    # Find interface block
    interface_match = re.search(r'interface\s+(\w+)\s*\{([^}]+)\}', mojom_content, re.DOTALL)
    if not interface_match:
        return None, []
    
    interface_name = interface_match.group(1)
    interface_body = interface_match.group(2)
    
    # Find all method declarations
    method_pattern = r'(\w+)\s*\(([^)]*)\)\s*(?:=>\s*\(([^)]*)\))?;'
    for match in re.finditer(method_pattern, interface_body):
        method_name = match.group(1)
        params_str = match.group(2).strip()
        response_str = match.group(3).strip() if match.group(3) else ""
        
        # Parse parameters
        params = []
        if params_str:
            for param in params_str.split(','):
                param = param.strip()
                if param:
                    parts = param.split()
                    if len(parts) >= 2:
                        param_type = parts[0]
                        param_name = parts[1]
                        params.append({'type': param_type, 'name': param_name})
        
        # Parse response parameters
        resp_params = []
        if response_str:
            for param in response_str.split(','):
                param = param.strip()
                if param:
                    parts = param.split()
                    if len(parts) >= 2:
                        param_type = parts[0]
                        param_name = parts[1]
                        resp_params.append({'type': param_type, 'name': param_name})
        
        methods.append({
            'name': method_name,
            'params': params,
            'resp_params': resp_params,
            'has_response': len(resp_params) > 0
        })
    
    return interface_name, methods

def generate_wrapper(interface_name, methods):
    """Generate C++ wrapper code."""
    code = f'''/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Auto-generated wrapper for {interface_name}
 * DO NOT EDIT - Generated from .mojom file
 * 
 * This wrapper provides clean, named parameters and hides the internal
 * t1/t2 serialization details of the Mojo bindings.
 */
#pragma once

#include <memory>
#include <stdexcept>
#include "../../../include/libcamera/ipa/softisp_interface.h"

namespace libcamera {{
namespace ipa {{
namespace softisp {{

class {interface_name}Wrapper {{
public:
    explicit {interface_name}Wrapper(std::unique_ptr<{interface_name}> iface)
        : interface_(std::move(iface)) {{
        if (!interface_) {{
            throw std::runtime_error("{interface_name} cannot be null");
        }}
    }}

    ~{interface_name}Wrapper() = default;

    // Prevent copying
    {interface_name}Wrapper(const {interface_name}Wrapper&) = delete;
    {interface_name}Wrapper& operator=(const {interface_name}Wrapper&) = delete;

    // --- Public API with Clean Names (No t1/t2) ---
'''
    
    for method in methods:
        # Build parameter list
        param_list = ", ".join([f"{p['type']} {p['name']}" for p in method['params']])
        param_names = ", ".join([p['name'] for p in method['params']])
        
        # Determine return type and expression
        if method['has_response']:
            if len(method['resp_params']) == 1:
                return_type = method['resp_params'][0]['type']
                return_expr = f"return interface_->{method['name']}({param_names})"
            else:
                return_type = "void"  # Simplified for multiple returns
                return_expr = f"interface_->{method['name']}({param_names})"
        else:
            return_type = "void"
            return_expr = f"interface_->{method['name']}({param_names})"
        
        # Add comment explaining the mapping
        param_map = ", ".join([f"{p['name']} (t{idx+1})" for idx, p in enumerate(method['params'])])
        
        code += f'''
    /**
     * {method['name']}
     * Mapping: {param_map}
     */
    {return_type} {method['name']}({param_list}) {{
        // Mapping is automatic: {param_map}
        {return_expr};
    }}
'''
    
    code += '''
private:
    std::unique_ptr<''' + interface_name + '''> interface_;
};

}} // namespace ipa::softisp
}} // namespace libcamera
}} // namespace libcamera
'''
    
    return code

def main():
    parser = argparse.ArgumentParser(description='Generate C++ wrapper from Mojom file')
    parser.add_argument('--mojom-file', required=True, help='Path to the .mojom file')
    parser.add_argument('--output-dir', required=True, help='Output directory for generated files')
    args = parser.parse_args()

    # Read the mojom file
    with open(args.mojom_file, 'r') as f:
        mojom_content = f.read()
    
    # Parse methods
    interface_name, methods = parse_mojom_methods(mojom_content)
    if not interface_name:
        print("Error: Could not parse interface from mojom file")
        sys.exit(1)
    
    if not methods:
        print("Warning: No methods found in interface")
    
    # Generate wrapper
    wrapper_code = generate_wrapper(interface_name, methods)
    
    # Write output
    base_name = os.path.basename(args.mojom_file).replace('.mojom', '')
    output_path = os.path.join(args.output_dir, f'{base_name}_wrapper.h')
    
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w') as f:
        f.write(wrapper_code)
    
    print(f"Generated wrapper: {output_path}")
    print(f"  Interface: {interface_name}")
    print(f"  Methods: {len(methods)}")

if __name__ == '__main__':
    main()
