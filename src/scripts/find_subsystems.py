#!/usr/bin/env python2.7

import argparse
import json
import re
import subprocess

file_subsys_cache = {}
addr_line_cache = {}

# Code to be included at the top of the subsystems header.
rscfl_subsys_header_top = \
"""#ifndef _RSCFL_SUBSYS_H_
#define _RSCFL_SUBSYS_H_

typedef enum {
"""

#Code at the bottom of the subsystems header.
rscfl_subsys_header_bottom = """
  NUM_SUBSYSTEMS
} rscfl_subsys;

#endif /* _RSCFL_SUBSYS_H_ */
"""


def get_subsys(addr, addr2line, linux, build_dir):
    # Use addr2line to convert addr to the filename that it is in.
    #
    # Args:
    #     addr: an address into the linux kernel.
    #     addr2line: an already-opened subprocess to addr2line, which we can
    #         use to map addr to a source code file.
    #     linux: string location of the Linux kernel.
    # Returns:
    #     A string representing the name of the subsystem that addr is located
    #     in.


    # Have we already mapped addr to a file name? If so use the cached value,
    # to save expensive calls out to addr2line.
    if addr in addr_line_cache:
        file_name = addr_line_cache[addr]
    else:
        addr2line.stdin.write("%s\n" % addr)
        file_line = addr2line.stdout.readline()
        if file_line.startswith("??:"):
            # addr2line can't map this address.
            return None
        # Convert file:line to file
        file_name = file_line.split(":")[0]

        # Make filename relative
        try:
            file_name = file_name.split("%s/" % build_dir)[1]
        except IndexError:
            # Generated filenames are already relative.
            pass
        addr_line_cache[addr] = file_name

    # Check the cache of files we've already found a subsystem for.
    if file_name in file_subsys_cache:
        return file_subsys_cache[file_name]
    else:
        # Use the get_maintainer.pl script to check the MAINTAINERS file,
        # to get the subsystem.
        proc = subprocess.Popen(["%s/scripts/get_maintainer.pl" % linux,
                                 "--subsystem", "--noemail",
                                 "--no-remove-duplicates", "--no-rolestats",
                                 "-f", "%s" % file_name], cwd=linux,
                                stdout=subprocess.PIPE)
        (stdout, stderr) = proc.communicate()
        maintainers = stdout.strip().split("\n")
        subsys = ""
        # The most specific subsystem is listed straight after linux-kernel
        # mailing list.
        for i, line in enumerate(maintainers):
            if line.startswith("linux-kernel@vger.kernel.org"):
                subsys = maintainers[i+1]
                break
        file_subsys_cache[file_name] = subsys
        return subsys


def get_addresses_of_boundary_calls(linux, build_dir):
    # Find the addresses of all call instructions that operate across a kernel
    # subsystem boundary.
    #
    # Args:
    #     linux: string representing the location of the linux kernel.
    #
    # Returns:
    #     a dictionary (indexed by subsystem name) where each element is a list
    #     of addresses that are callq instructions whose target is in the
    #     appropriate subsystem.
    boundary_fns = {}
    addr2line = subprocess.Popen(['addr2line', '-e' '%s/vmlinux' % linux],
                                 stdout=subprocess.PIPE,
                                 stdin=subprocess.PIPE)
    # Use objdump+grep to find all callq instructions.
    proc = subprocess.Popen('objdump -d vmlinux', shell=True,
                            stdout=subprocess.PIPE, cwd=linux)
    # regex to match callq instructions, creating groups from the caller, and
    # callee addresses.
    p_callq = re.compile("([0-9a-f]{16,16}).*callq.*([0-9a-f]{16,16}).*$")
    for line in proc.stdout:
        m = p_callq.match(line)
        if m:
            caller_addr = m.group(1)
            callee_addr = m.group(2)

            caller_subsys = get_subsys(caller_addr, addr2line, linux, build_dir)
            callee_subsys = get_subsys(callee_addr, addr2line, linux, build_dir)
            if not caller_subsys:
                # Address that we can't map to source file.
                continue
            if callee_subsys != caller_subsys and callee_subsys is not None:
                if callee_subsys not in boundary_fns:
                    boundary_fns[callee_subsys] = []
                boundary_fns[callee_subsys].append(caller_addr)
    return boundary_fns


def append_to_rscfl_subsys_json(rscfl_subsys_json, subsys_names):
    # Add new subsystems to a JSON file.
    #
    # Parses rscfl_subsys_json, and adds any subsystems in subsys_entries to
    # the file.
    #
    # Args:
    #     rscfl_subsys_json: a file object that can be read, and written. If the
    #         file contains a valid JSON structure, new subsystems will be
    #         appended. Otherwise, all subsystems will be dumped to the file.
    #     subsys_names: a list of string names of Linux subsystems.
    try:
        json_entries = json.load(rscfl_subsys_json)
    except ValueError:
        # No valid JSON in the file.
        json_entries = {}

    for subsys in subsys_names:
        if subsys not in json_entries:
            # Remove various bits of punctuation so we can index using the name.
            clean_subsys_name = re.sub(r'\W+', '', subsys)
            json_entries[clean_subsys_name] = {}
            json_entries[clean_subsys_name]['index'] = len(json_entries)
            # long_name is used to deduplicate subsystems. Its value should not
            # be modified in the ouputted JSON file.
            json_entries[clean_subsys_name]['long_name'] = subsys
            # short_name is used as a key in enums. Its value can be modified to
            # be more human/code-friendly.
            json_entries[clean_subsys_name]['short_name'] = clean_subsys_name

    json.dump(json_entries, rscfl_subsys_json, indent=2)


def generate_rscfl_subsystems_header(json_file, header_file):
    # Using the JSON list of subsystems, generate a header file that creates
    # a enum of subsystems.
    # Save this header file to $header_file
    #
    # Args:
    #     json_file: File object with a JSON list of subsystems.
    #     header_file: File to write a C header file containing an enum of
    #         possible subsystems.
    json_file.seek(0)
    subsystems = json.load(json_file)
    header_file.write(rscfl_subsys_header_top)
    for i, subsystem in enumerate(subsystems):
        header_file.write("  %s=%d,\n" % (subsystem, i))
    header_file.write(rscfl_subsys_header_bottom)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', dest='linux_root', action='store', default='.',
                        help="""location of the root of the
                        Linux source directory.""")
    parser.add_argument('--build_dir', help="""Location that vmlinux was
                        built in.""")
    parser.add_argument('--find_subsystems', action='store_true')
    parser.add_argument('-J', dest='rscfl_subsys_json',
                        type=argparse.FileType('r+'), help="""JSON file to write
                        subsystems to.""")
    parser.add_argument('--update_json', action='store_true')
    parser.add_argument('--gen_header', type=argparse.FileType('w'))

    args = parser.parse_args()

    # If we havent' been given an explicit build directory, it is fair to
    # assume that the kernel was built in the source directory.
    if args.build_dir:
        build_dir = args.build_dir
    else:
        build_dir = args.linux_root
    if args.update_json or args.find_subsystems:
        subsys_entries = get_addresses_of_boundary_calls(args.linux_root,
                                                         build_dir)

    if args.update_json:
        append_to_rscfl_subsys_json(args.rscfl_subsys_json,
                                    subsys_entries.keys())

    if args.gen_header:
        generate_rscfl_subsystems_header(args.rscfl_subsys_json,
                                         args.gen_header)

    if args.find_subsystems:
        for subsys in subsys_entries:
            entry_points = ['kprobe.statement(0x%s).absolute,' % x for x in
                            subsys_entries[subsys]]
            entry_points[-1] = entry_points[-1][0:-1]
            print("probe ")
            print("\n".join(entry_points))
            print("""
{
    print("Entered %s subsystem")
}
              """ % subsys)


if __name__ == '__main__':
    main()