#!/usr/bin/env python
import sys
import os
top_level_dir = os.path.realpath(__file__ + '/../../../../') + "/"
camkes_path = top_level_dir + "tools/camkes/"
sys.path.insert(0, camkes_path)
import camkes.parser as parser
import camkes.ast as ast
import copy
import re
import shutil
import getopt

from debug_config import SERIAL_IRQ_NUM, SERIAL_PORTS

DEFINITIONS_DIR = os.path.realpath(__file__ + '/../include/definitions.camkes')
TEMPLATES_SRC_DIR = os.path.realpath(__file__ + '/../templates') + "/"
DEBUG_CAMKES = os.path.realpath(__file__ + '/../include/debug.camkes')

def main(argv):
    # Parse input
    project_camkes, plat, arch, vm_mode, vm = parse_args(argv)
    # Open camkes file for parsing
    with open(project_camkes) as camkes_file:
        lines = camkes_file.readlines()
    # Save any imports and add them back in
    imports = ""
    import_regex = re.compile(r'import .*')
    for line in lines:
        if import_regex.match(line):
            imports += line

    camkes_text = "\n".join(lines)
    
    camkes_builtin_path = camkes_path + 'include/builtin'
    include_path = [camkes_builtin_path]
    if vm_mode:
        print "vm mode"
        cpp = True
        config_path = top_level_dir + 'apps/%s/configurations' % vm
        vm_components_path = top_level_dir + 'apps/%s/../../components/VM' % vm
        plat_includes = top_level_dir + 'kernel/include/plat/%s' % plat
        cpp_options  = ['-DCAMKES_VM_CONFIG=%s' % vm, "-I"+config_path, "-I"+vm_components_path, "-I"+plat_includes]
        include_path.append(top_level_dir + "/projects/vm/components")
        include_path.append(top_level_dir + "projects/vm/interfaces")
    else:
        cpp = False
        cpp_options = []
    # Parse using camkes parser
    target_ast = parser.parse_to_ast(camkes_text, cpp, cpp_options)  
    # Resolve other imports
    project_dir = os.path.dirname(os.path.realpath(project_camkes)) + "/"
    target_ast, _ = parser.resolve_imports(target_ast, project_dir, include_path, cpp, cpp_options)
    target_ast = parser.resolve_references(target_ast)
    # Find debug components declared in the camkes file
    debug_components, hierarchical_components = get_debug_components(target_ast)
    if debug_components:
        # Use the declared debug components to find the types we must generate
        debug_types = get_debug_component_types(target_ast, debug_components)
        # Generate the new types
        target_ast = create_debug_component_types(target_ast, debug_types)
        # Add declarations for the new types
        target_ast = add_debug_declarations(target_ast, debug_components, hierarchical_components)
        # Get the static definitions needed every time
        debug_definitions = get_debug_definitions()
        # Generate server based on debug components
        debug_definitions += generate_server_component(debug_components)
        # Update makefile with the new debug camkes
        update_makefile(project_camkes, debug_types)
        # Copy the templates into the project directory
        copy_templates(project_camkes)
        # Add our debug definitions
        new_camkes = parser.pretty(parser.show(target_ast))
        # Reparse and rearrange the included code
        procedures = []
        main_ast = []
        new_ast = parser.parse_to_ast(new_camkes, cpp, cpp_options)
        for component in new_ast:
            if isinstance(component, ast.Objects.Procedure):
                procedures.append(component)
            else:
                main_ast.append(component)   
        final_camkes = imports + debug_definitions + parser.pretty(parser.show(procedures + main_ast))
        name_regex = re.compile(r"apps/(.*)/")
        search = name_regex.search(project_camkes)
        # Write a gdbinit file
        write_gdbinit(search.group(1), debug_components, plat, arch)
        # Write out the new camkes file
        with open(project_camkes + ".dbg", 'w') as f:
            for line in final_camkes:
                f.write(line)
    else:
        print " No debug components found"

# Find debug components declared in the camkes file
def get_debug_components(target_ast):
    debug = dict()
    hierarchical_components = dict()
    for context in target_ast:
        if isinstance(context, ast.Objects.Assembly) or isinstance(context, ast.Objects.Component):
            for config in context.children():
                if isinstance(config, ast.Objects.Configuration):
                    for setting in config.settings:
                        if setting.attribute == 'debug' and setting.value == "\"True\"":
                            debug[setting.instance] = context
                            # If hierarchical, then keep track of the component to expose interfaces later
                            if isinstance(context, ast.Objects.Component):
                                if context not in hierarchical_components:
                                    hierarchical_components[context] = []
                                hierarchical_components[context].append(setting.instance)
                            config.settings.remove(setting)
    return debug, hierarchical_components

# Use the declared debug components to find the types we must generate
def get_debug_component_types(target_ast, debug_components):
    debug_types = dict()
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly) or isinstance(assembly, ast.Objects.Component):
            for composition in assembly.children():
                if isinstance(composition, ast.Objects.Composition):
                    for instance in composition.children():
                        if isinstance(instance, ast.Objects.Instance):
                            # Go through each declared component, 
                            # and check if it is a debug component
                            if instance.name in debug_components:
                                type_ref = instance.children()[0]
                                type_name = copy.copy(type_ref._symbol)
                                debug_types[type_name] = type_ref._symbol

    return debug_types

# Generate the new types
def create_debug_component_types(target_ast, debug_types):
    for index, component in enumerate(target_ast):
        if isinstance(component, ast.Objects.Component):
            # Find the debug types and copy the definition
            if component.name in debug_types:
                debug_component = copy.copy(component)
                debug_component.name = debug_types[component.name]
                # Add the necessary interfaces
                new_interface = "uses CAmkES_Debug fault;\n"
                new_interface += "provides CAmkES_Debug GDB_delegate;\n"
                new_interface += "uses CAmkES_Debug GDB_mem;\n"
                new_interface += "provides CAmkES_Debug GDB_mem_handler;\n"
                # Get the component as a string and re-parse it with the new interfaces
                string = debug_component.__repr__().split()
                string[-1:-1] = new_interface.split()
                string = " ".join(string)
                debug_ast = parser.parse_to_ast(string)
                # Replace the debug component
                target_ast[index] = debug_ast[0]
    return target_ast

# Add the debug declarations to the ast
def add_debug_declarations(target_ast, debug_components, hierarchical_components):
    debug_interfaces, debug_instances, debug_connections, \
    assembly_instances, assembly_connections = generate_debug_objects(debug_components, target_ast, hierarchical_components)
    # Edit the actual AST
    for context in target_ast:
        # Add new debug objects to contexts
        if context in debug_instances and not isinstance(context, ast.Objects.Assembly):
            context.provides += debug_interfaces[context][0]
            context.emits += debug_interfaces[context][1]
            for obj in context.children():
                if isinstance(obj, ast.Objects.Composition):
                    obj.instances += debug_instances[context]
                    obj.connections += debug_connections[context]
        # Add top-level assembly specific objects
        elif isinstance(context, ast.Objects.Assembly):
            for obj in context.children():
                # Add top level assembly specific code
                if isinstance(obj, ast.Objects.Composition):
                    obj.instances += assembly_instances
                    obj.connections += assembly_connections
                # Add debug configuration options
                elif isinstance(obj, ast.Objects.Configuration):
                    serial_irq = ast.Objects.Setting("debug_hw_serial", "irq_attributes", \
                                                     SERIAL_IRQ_NUM)
                    obj.settings.append(serial_irq)
                    serial_attr = ast.Objects.Setting("debug_hw_serial", "serial_attributes",\
                                                      "\"%s\"" % SERIAL_PORTS)
                    obj.settings.append(serial_attr)
    return target_ast

def generate_debug_objects(debug_components, target_ast, hierarchical_components):
    with open(DEBUG_CAMKES) as debug_file:
            assembly_text = debug_file.readlines()
    debug_interfaces = dict()
    debug_instances = dict()
    debug_connections = dict()
    debug_text = dict()
    i = 0
    for component, context in debug_components.iteritems():
        if context not in debug_text:
             debug_text[context] = ["composition {", "}"]
             debug_interfaces[context] = [[], []]
         # If it's a component, then break out RPC
        if isinstance(context, ast.Objects.Component):
            debug_text[context].insert(1, """connection ExportRPC debug%d_delegate(from %s_delegate_interface,
                                            to %s.GDB_delegate);""" % (i, component, component))
            debug_text[context].insert(1, "connection ExportRPC debug%d(from %s.fault, to %s_debug_interface);"
                                            % (i, component, component))
            debug_text[context].insert(-1, "connection seL4GDBMem debug%d_mem(from %s.GDB_mem," % (i, component) +
                                 "to %s.GDB_mem_handler);" % (component))
            # Add the declarations for the component interfaces
            debug_interfaces[context][0].append("provides CAmkES_Debug %s_delegate_interface;" % component)
            debug_interfaces[context][1].append("emits CAmkES_Debug %s_debug_interface;" % component)
            i += 1
        # If it's an assembly, then we're at the top level
        elif isinstance(context, ast.Objects.Assembly):
            assembly_text.insert(-1, """connection seL4Debug debug%d_delegate(from debug.%s_GDB_delegate,
                                     to %s.GDB_delegate);""" % (i, component, component))
            assembly_text.insert(-1, "connection seL4GDB debug%d(from %s.fault, to debug.%s_fault);"
                                      % (i, component, component))
            assembly_text.insert(-1, "connection seL4GDBMem debug%d_mem(from %s.GDB_mem," % (i, component) +
                                 "to %s.GDB_mem_handler);" % (component))
            i += 1
    i = 0
    # Add hierarchical declarations at top level
    for context in target_ast:
        if isinstance(context, ast.Objects.Assembly):
            for obj in context.children():
                if isinstance(obj, ast.Objects.Composition):
                    for obj2 in obj.children():
                        if isinstance(obj2, ast.Objects.Instance) and obj2.type._referent in hierarchical_components:
                            for component in hierarchical_components[obj2.type._referent]:
                                assembly_text.insert(-1, """connection seL4Debug %s_debug%d_delegate(from debug.%s_GDB_delegate,
                                                         to %s.%s_delegate_interface);""" % (obj2.name, i, component, obj2.name, component))
                                assembly_text.insert(-1, "connection seL4GDB debug%d(from %s.%s_debug_interface, to debug.%s_fault);"
                                                            % (i, obj2.name, component, component))
                                i += 1
    # Create objects to add for each context
    for context, text in debug_text.iteritems():
        camkes_text = "\n".join(debug_text[context])
        debug_ast = parser.parse_to_ast(camkes_text)
        debug_instances[context] = debug_ast[0].instances
        debug_connections[context] = debug_ast[0].connections
    # Create objects for top-level assembly
    assembly_text = "\n".join(assembly_text)
    assembly_ast = parser.parse_to_ast(assembly_text)
    assembly_instances = assembly_ast[0].instances
    assembly_connections = assembly_ast[0].connections
    return debug_interfaces, debug_instances, debug_connections, assembly_instances, assembly_connections


# Generates the root debug server, with connections to each of the components being debugged
def generate_server_component(debug_components):
    server = ""
    server += "component debug_server {\n"
    server += "  uses IOPort serial_port;\n"
    server += "  consumes IRQ%s serial_irq;\n" % SERIAL_IRQ_NUM
    for component in debug_components:
        server += "  uses CAmkES_Debug %s_GDB_delegate;\n" % component
        server += "  provides CAmkES_Debug %s_fault;\n" % component
    server += "}\n"
    return server

def get_debug_definitions():
    with open(DEFINITIONS_DIR) as definitions_file:
        definitions_text = definitions_file.read() % SERIAL_IRQ_NUM
    return definitions_text

def update_makefile(project_camkes, debug_types):
    project_dir = os.path.dirname(os.path.realpath(project_camkes)) + "/"
    if not os.path.isfile(project_dir + "Makefile.bk"):
        # Read makefile
        with open(project_dir + "Makefile", 'r+') as orig_makefile:
            makefile_text = orig_makefile.readlines()
        # Backup Makefile
        with open(project_dir + "Makefile.bk", 'w+') as bk_makefile:
            for line in makefile_text:
                bk_makefile.write(line)
        # Search for the ADL and Templates liat and modify them
        templates_found = False
        adl_regex = re.compile(r"ADL")
        template_regex = re.compile(r'TEMPLATES :=')
        for index, line in enumerate(makefile_text):
            if template_regex.search(line):
                # Add the debug templates
                makefile_text[index] = line.rstrip() + " debug\n"
                templates_found = True
            if adl_regex.search(line):
                # Change the CDL target
                makefile_text[index] = line.rstrip() + ".dbg\n"
        makefile_text.append("\n\n")
        # Add templates if they didn't define their own
        new_lines = list()
        if not templates_found:
            new_lines.append("TEMPLATES := debug\n")
        # Write out new makefile
        makefile_text = new_lines + makefile_text
        with open(project_dir + "Makefile", 'w') as new_makefile:
            for line in makefile_text:
                new_makefile.write(line)

# Cleanup any new files generated
def clean_debug(project_camkes):
    project_dir = os.path.dirname(os.path.realpath(project_camkes)) + "/"
    if os.path.isfile(project_dir + "Makefile.bk"):
        os.remove(project_dir + "Makefile")
        os.rename(project_dir + "Makefile.bk", 
                  project_dir + "Makefile")
    if os.path.isfile(project_camkes + ".dbg"):
        os.remove(project_camkes + ".dbg")
    if os.path.isdir(project_dir + "debug"):
        shutil.rmtree(project_dir + "debug")
    if os.path.isfile(top_level_dir + ".gdbinit"):
        os.remove(top_level_dir + ".gdbinit")

# Copy the templates to the project folder
def copy_templates(project_camkes):
    project_dir = os.path.dirname(os.path.realpath(project_camkes)) + "/"
    if not os.path.exists(project_dir + "debug"):
        shutil.copytree(TEMPLATES_SRC_DIR, project_dir + "debug")

# Write a .gdbinit file
# Currently only loads the symbol table for the first debug component
def write_gdbinit(projects_name, debug_components, arch, plat):
    with open(top_level_dir + '.gdbinit', 'w+') as gdbinit_file:
        component_name = debug_components.keys()[0]
        gdbinit_file.write("symbol-file build/%s/%s/%s/%s.instance.bin\n" % 
                (plat, arch, projects_name, component_name))
        gdbinit_file.write("target remote :1234\n")

def parse_args(argv):
    vm_mode = False
    vm = None
    try:
        opts, args = getopt.getopt(argv, "cmv:p:a:")
    except getopt.GetoptError as err:
        print str(err)
        sys.exit(1)
    if len(args) == 0:
        print "Not enough arguments"
        sys.exit(1)
    elif len(args) > 1:
        print args
        print "Too many args"
        sys.exit(1)
    else:
        project_camkes = args[0]
        if not os.path.isfile(project_camkes):
            print "File not found: %s" % project_camkes
            sys.exit(1)
    for opt, arg in opts:
        if opt == "-c":
            clean_debug(project_camkes)
            sys.exit(0)
        if opt == "-v":
            vm_mode = True
            vm = arg
        if opt == "-p":
            plat = arg.lstrip()
        if opt == "-a":
            arch = arg.lstrip()
    return project_camkes, plat, arch, vm_mode, vm

if __name__ == "__main__":
    main(sys.argv[1:])