#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

'''Standalone RPC stub generator. This tool is intended to be used for
non-CAmkES systems when you would still like to leverage the CAmkES IDL for
describing interfaces between your processes. You are expected to provide your
own templates, using the same Jinja syntax as for CAmkES templates.'''

import argparse, sys
import camkes.ast as ast
import camkes.parser as parser
from Render import render
from Simplify import simplify

def main():
    p = argparse.ArgumentParser('standalone RPC stub generator')
    p.add_argument('--input', '-i', type=argparse.FileType('r'),
        default=sys.stdin, help='interface specification')
    p.add_argument('--output', '-o', type=argparse.FileType('w'),
        default=sys.stdout, help='file to write output to')
    p.add_argument('--procedure', '-p', help='procedure to operate on ' \
        '(defaults to the first available)')
    p.add_argument('--template', '-t', type=argparse.FileType('r'),
        required=True, help='template to use')
    opts = p.parse_args()

    # Parse the input specification.
    try:
        spec = parser.parse_to_ast(opts.input.read())
    except parser.CAmkESSyntaxError as e:
        print >>sys.stderr, 'syntax error: %s' % str(e)
        return -1

    # Find the procedure the user asked for. If they didn't explicitly specify
    # one, we just take the first.
    for obj in spec:
        if isinstance(obj, ast.Procedure):
            if opts.procedure is None or opts.procedure == obj.name:
                proc = obj
                break
    else:
        print >>sys.stderr, 'no matching procedure found'
        return -1

    # Simplify the CAmkES AST representation into a representation involving
    # strings and simpler objects.
    proc = simplify(proc)

    # Render the template.
    print >>opts.output, render(proc, opts.template.read())

    return 0

if __name__ == '__main__':
    sys.exit(main())
