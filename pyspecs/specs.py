import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import subprocess
import time
from .specsparse import *

"""
Ideas:
    - generate value files
    - generate netlists based on graph representations
    - simulate
    - paralel run
    - others?

"""

default_settings = {
    "o": "traces/delete_me.vcd",
    "abstol": 1e-8,
    "reltol": 1e-4
}



def create_arguments_netlist(netlist_file, output_file, abstol, reltol):
    """ Function called by specs.simulate(), generates
    the argument string for the command-line tool.

    Returns:
        arguments (list of str):
            Formatted list of arguments required by SPECS
    """

    arguments_dictionary = { "f":netlist_file,
                             "o":output_file,
                             "abstol":abstol,
                             "reltol":reltol
                           }

    arguments = create_arguments_dictionary(arguments_dictionary)

    return arguments


def create_arguments_dictionary(arguments_dictionary:dict):
    """ Function called by specs.simulate(), generates
    the argument string for the command-line tool.

    Args:
        arguments_dictionary (dict):
            The keys of this dictionary are keywords expected
            by SPECS, while their contents hold their value.

            Example: {"f":"circuit.txt", "o":"data.vcd","
                      "abstol":1e-8, "reltol":1e-4
                     }

    Returns:
        arguments (list of str):
            Formatted list of arguments required by SPECS
    """

    arguments = []

    for keyword_string in arguments_dictionary:
        value = arguments_dictionary[keyword_string]

        # Long keywords are prepended with double dash
        if len(keyword_string) > 1:
            keyword_string = "--" + keyword_string
        else:
            keyword_string = "-" + keyword_string

        arguments.append(keyword_string)
        if value is not None:
            arguments.append(str(value))

    return arguments

def run_and_time(simulator_command, arguments, verbose):
    """Function that runs SPECS and times it.

    This is BLOCKING execution, so not intended for paralel
    usage. For this case, use par_run().

    Args:
        simulator_command (str):
            The command to call SPECS comprised of
            its directory and simulator name. Ex: ./specs
        arguments (str):
            Command-line arguments passed to SPECS
        verbose (bool):
            If true, will print SPECS output to the Python shell
    Returns:
        time_ms:
            Execution time of SPECS for that run, in miliseconds (ms)
    """

    print("Executing SPECS from python. . .")

    #spec_env = {"SC_COPYRIGHT_MESSAGE":"DISABLE"}

    process_call = simulator_command + arguments

    tic_ns = time.perf_counter_ns()
    print("Command: ", ' '.join(process_call))
    #p = subprocess.Popen(process_call, env=spec_env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    p = subprocess.Popen(process_call, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    specs_stdout, specs_stderr = p.communicate()
    p.wait()
    toc_ns = time.perf_counter_ns()

    time_ms = (toc_ns - tic_ns)*1e-6

    specs_return_code = p.returncode
    print("Execution finished with code: ", specs_return_code)

    if (not specs_return_code == 0) or (verbose):
        print("SPECS had an error. Here is the full output: ")
        for line in specs_stdout.splitlines():
            print(line.decode(encoding='utf-8', errors='ignore'))

    return time_ms, (specs_return_code, specs_stdout)

def simulate(netlist_file=None,
             custom_arguments=None,
             simulator_directory="",
             verbose=False,
             **kwargs):
    """Function that call SPECS from the Python shell

    Args:
        netlist_file (str):
            File containing the netlist description
            of the circuit to simulate. Should contain
            a simulation directive within it (.tran, .dc, .op).

            The netlist is a text file that can be:
                - Created using the KiCAD library of components in
                  its schematic editor (eeschema)
                - Created and edited by hand by knowing the syntax
                - Generated proceduraly using scripts. It's a basic .txt

            It can be achieved with custom arguments by defining
            {"f": "path/to/file"} in the dict custom_arguments.

            Defaults to "".

        output_file (str, optional):
            VCD file containing all the recorded events.
            Can be read with GTKWAVE or parsed with parse_vcd()
            Defaults to "traces/delete_me.vcd".

        abstol (double, optional):
            Field absolute tolerance for propagating events in the simulator.
            Is relevant when running circuits with feedback.

            It can be achieved with custom arguments by defining
            {"abstol": 1e-8} in the dict custom_arguments.

            Defaults to 1e-8.

        reltol (double, optional):
            Field relative tolerance for propagating events in the simulator.
            Is relevant when running circuits with feedback.

            It can be achieved with custom arguments by defining
            {"reltol": 1e-4} in the dict custom_arguments.

            Defaults to 1e-4.

        custom_arguments (dict, optional):
                         <<< ADVANCED USE >>>
            It is possible to call SPECS with many other arguments.
            You can consult them by running specs without arguments
            in the command line interface.

            If custom_arguments is defined, you need to specify
            the netlist as a part of the dictionary.

            There is no specific order that the arguments must follow.

        simulator_directory (str, optional):
            SPECS can either be included in the PATH variable
            or exist as a standalone executable. In the latter case,
            it is necessary to define its location.

            Examples:
                simulator_directory = "./"
                    Means that the executable is in the same
                    directory as this script."
                simulator_directory = "/home/user/Documents/"
                    Means that the SPECS executable is in that
                    specific path.

            Defaults to "".

        verbose (bool, optional):
            If true, will print all the outputs generated by SPECS
            to the Python shell.
    """

    assert((netlist_file is not None) or (custom_arguments is not None))


    if 'abstol' in kwargs:
        abstol = kwargs['abstol']
    else:
        abstol = default_settings['abstol']

    if 'reltol' in kwargs:
        reltol = kwargs['reltol']
    else:
        reltol = default_settings['reltol']

    if 'output_file' in kwargs:
        output_file = kwargs['output_file']
    else:
        output_file = default_settings['o']

    arguments = []
    if (netlist_file is not None):
        arguments = create_arguments_netlist(netlist_file, output_file, abstol, reltol)

    if (custom_arguments is not None):
        current_settings = default_settings.copy()
        current_settings.update(custom_arguments) # overwrites defaults with the user input where needed
        arguments += create_arguments_dictionary(current_settings)

    simulator_name = "specs"
    simulator_command = [simulator_directory + simulator_name]

    outputs = run_and_time(simulator_command, arguments, verbose)

    return outputs
