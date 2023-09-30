from pyDigitalWaveTools.vcd.parser import VcdParser
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import json

def parse_vcd_dataframe(filename):
    """Parses a SPECS VCD file and returns simulation data as a DataFrame.

    Args:
        filename (str):
            The path to the VCD file.

    Returns:
        simulation_data_df:
            The simulation data with signals as columns and time/wavelength as rows.
    """
    with open(filename) as vcd_file:
        vcd = VcdParser()
        vcd.parse(vcd_file)
        data = vcd.scope.toJson()

    # print(json.dumps(data, indent=4, sort_keys=True))
    simulation_data_df = pd.DataFrame() # initialize empty dataframe

    top_level_name = 'SystemC'
    top_scope = None
    for child in data['children']:
        if is_scope(child) and child['name'] == top_level_name:
            top_scope = child
            break
    if top_scope is None:
        raise RuntimeError(f'Top level scope not found in {filename}')

    assert('name' in top_scope.keys())
    assert('type' in top_scope.keys())
    assert('children' in top_scope.keys())
    assert(top_scope['type']['name'] == 'struct')

    scopes = []
    probes = []
    pdets = []
    values = []
    for child in top_scope['children']:
        if is_scope(child):
            scopes.append(child)
            if is_probe(child):
                probes.append(child)
            if is_pdet(child):
                pdets.append(child)
        else:
            values.append(child)

    # print(f'top level values: {[x["name"] for x in values]}')
    # print(f'top level scopes: {[x["name"] for x in scopes]}')
    # print(f'top level probes: {[x["name"] for x in probes]}')

    is_wl_sweep = check_wl_sweep(probes)
    has_wl = False
    save_wl = False

    df_probes = pd.DataFrame()
    if len(probes) > 0:
        for i,probe in enumerate(probes):
            probe_name = probe['name']
            for j,signal in enumerate(probe['children']):
                signal_name = signal['name']
                full_name = probe_name + '/' + signal_name

                save_wl = is_wl_sweep and not has_wl
                if (not signal_name=='wavelength' or (signal_name=='wavelength' and save_wl)):
                    current_signal_df = pd.DataFrame()
                    current_signal_df['tick'] = vcd_get_ticks_vector(signal)

                    if signal_name=='wavelength':
                        has_wl = False # this variable set to true will save WL vector only once
                        full_name = signal_name # wavelength should be saved independent of probe

                    current_signal_df[full_name] = vcd_get_data_vector(signal)

                if i==0 and j==0:
                    df_probes = current_signal_df
                else:
                    df_probes = df_probes.merge(current_signal_df, how='outer')

        df_probes = df_probes.sort_values('tick')
        df_probes = df_probes.reset_index(drop=True)

        if not is_wl_sweep: # making sure that time goes first
            df_probes.insert(0, 'time', df_probes['tick'] * vcd_get_multiplier(vcd))
        else: # making sure that wl goes first
            first_column = df_probes.pop('wavelength')
            df_probes.insert(0, 'wavelength', first_column)

        # we don't need ticks anymore
        df_probes = df_probes.drop(columns=['tick'])

        # getting rid of the NaNs: for time domain we repeat the last valid value,
        #                          for freq domain we interpolate linearly
        if is_wl_sweep:
            df_probes = df_probes.drop(df_probes.loc[df_probes['wavelength']== 0].index.values)
            df_probes = df_probes.interpolate()
        else:
            df_probes = df_probes.fillna(method='ffill')

    # Same with pdetectors
    df_pdet = pd.DataFrame()
    if not is_wl_sweep and len(pdets) > 0:
        for i,pdet in enumerate(pdets):
            pdet_name = pdet['name']
            for j,signal in enumerate(pdet['children']):
                if signal["type"]["name"] == "struct":
                    continue
                signal_name = signal['name']
                full_name = pdet_name + '/' + signal_name

                # Add data to a temporary DF
                current_signal_df = pd.DataFrame()
                current_signal_df['tick'] = vcd_get_ticks_vector(signal)
                current_signal_df[full_name] = vcd_get_data_vector(signal)

                if df_pdet.empty:
                    df_pdet = current_signal_df
                else:
                    df_pdet = df_pdet.merge(current_signal_df, how='outer')

        df_pdet = df_pdet.sort_values('tick')
        df_pdet = df_pdet.reset_index(drop=True)
        if False:
            # Add a point next to first value from photodetector
            # to ensure 'step' appearance (as its more correct)
            print(df_pdet.head())
            df_pdet.loc[-1] = df_pdet.loc[0]
            df_pdet.loc[0, 'tick'] = df_pdet.loc[1]['tick']
            df_pdet = df_pdet.sort_values('tick')
            df_pdet = df_pdet.reset_index(drop=True)
            print(df_pdet.head())

        # making sure that time goes first
        df_pdet.insert(0, 'time', df_pdet['tick'] * vcd_get_multiplier(vcd))

        # we don't need ticks anymore
        df_pdet = df_pdet.drop(columns=['tick'])

        # getting rid of the NaNs: for time domain we repeat the last valid value,
        df_pdet = df_pdet.fillna(method='ffill')


    print('--- Contents of simulation ---')
    print ('In df_pdet')
    for col in list(df_pdet.columns):
        print('\t',col)
    print ('In df_probes')
    for col in list(df_probes.columns):
        print('\t',col)
    if is_wl_sweep:
        print('Type of simulation: FD')
    else:
        print('Type of simulation: TD')

    print('------------------------------')

    return df_probes, df_pdet

def verify_wl_sweep(simulation_data):
    """Checks the type of simulation based on the processed data.

    Args:
        simulation_data (DataFrame):
            Data recovered using parse_vcd_xxxx methods.

    Returns:
        is_wl_sweep(bool):
            True if it is a wavelength sweep, False otherwise.
    """
    is_wl_sweep = False

    if isinstance(simulation_data,pd.DataFrame):
        is_wl_sweep = list(simulation_data.columns)[0] == 'wavelength'
    else:
        print('Error: Invalid simulation data')

    return is_wl_sweep

def find_by_time(simulation_data,colname,time):
    """Finds specific time values from the processed data
    Useful when you want to a datapoint of a specific moment
    but there was no transition there so it is not necessarily
    contained in the data frame.

    As it is an event-based simulator, it means that if
    the exact time that you want is not there, the closest
    previous value is the correct one, and this function gets it
    automatically for you.

    Args:
        simulation_data (DataFrame):
            Data recovered using parse_vcd_xxxx methods.
        colname (string):
            Name of the column that you want to extract data from.
        time (float):
            Time that you want to get your sample.

    Returns:
        value(float):
            Sample value at the specified time..
    """
    # first check if exact match is present
    # else, gets most recent value smaller than
    # the specified time

    value = None

    exact_match_df = simulation_data.loc[simulation_data['time'] == time]
    if exact_match_df.empty:
        previous_match_df = simulation_data.loc[simulation_data['time'] <= time]
        previous_match_df = previous_match_df.tail(1)
        value = previous_match_df[colname].values[0]
    else:
        value = exact_match_df[colname].values[0]

    return value

######### The next functions are intended for internal use #########

def is_scope(data):
    return data['type']['name'] == 'struct'

def is_pdet(data):
    if not is_scope(data):
        return False

    allowed_attrs = ["readout", "readout_no_interference"]
    for child in data['children']:
        if child['name'] not in allowed_attrs:
            return False
    return True

def is_probe(data):
    if not is_scope(data):
        return False

    allowed_attrs = ["wavelength", "power", "abs", "phase", "real", "imag"]
    for child in data['children']:
        if is_scope(child):
            # Probe contains only values
            return False
        base_name = child['name']
        # For MLprobe ignore what's after the "@"
        # If there is no "@", it remains the full name
        i_last_at = base_name.rfind('@')
        if i_last_at > 0:
            base_name = base_name[0:i_last_at]
        if base_name not in allowed_attrs:
            return False
    return True

def check_wl_sweep(probes):
    for probe in probes:
        # see if wavelength is part of probe attributes
        attrs = ["wavelength", "power", "abs", "phase"]
        lengths = {a:-1 for a in attrs}
        for signal in probe['children']:
            for a in attrs:
                if signal['name'] == a:
                    lengths[a] = len( signal['data'] )

        match = False
        if lengths[attrs[1]] != -1:
            match = lengths[attrs[0]] == lengths[attrs[1]]
        if lengths[attrs[2]] != -1:
            match = lengths[attrs[0]] == lengths[attrs[2]]

        if match and lengths[attrs[0]] != 1:
            return True
    return False

def vcd_get_data_vector(signal):
    data = np.array([float(tup[1][1::]) for tup in signal['data']])
    return data

def vcd_get_ticks_vector(signal):
    ticks = np.array([tup[0] for tup in signal['data']])
    return ticks

def vcd_get_multiplier(vcd):
    timescale_list = vcd.timescale.split()
    base_scale = float(timescale_list[0])
    unit = timescale_list[1][0]

    multipliers = {
        's': 1.0,
        'm': 1e-3,
        'u': 1e-6,
        'n': 1e-9,
        'p': 1e-12,
        'f': 1e-15,
    }

    # returns value in seconds
    return base_scale * multipliers[unit]
