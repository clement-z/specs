#!/bin/env bash

trace_file=$1
config_dir=$2

if [[ -d $config_dir && ! -z "$(ls $config_dir)" ]]; then
    configs=("`find $config_dir -iname '*.gtkw' -printf '%p '`")
    configs=(${configs[*]})

    # Check number of config file found
    if [ ${#configs[*]} -eq 1 ]; then
        # Only one file: run with it
        echo -e "Going with the only gtkwave config found: ${configs[0]}."
        echo "gtkwave -a \"${configs[0]}\" \"$1\""
        gtkwave -a "${configs[0]}" "$1"
    else
        # Several configs found, ask which to use
        echo "Found the following ${#configs[*]} gtkwave configs:"
        let "i = 1"
        echo -e "0:\tNone"
        for config in ${configs[*]}; do
            echo -e "$i:\t\"$config\""
            let "i = ++i"
        done

        echo -n "Your choice ? [0]: "

        # declare the choice variable as an integer
        declare -i choice
        declare -i i_config
        read choice
        let "i_config = choice - 1"

        while [[ $choice -lt 0 || $i_config -ge ${#configs} ]]; do
            echo -n "Out of bounds. Your choice ? [0]: "
            read choice
            let "i_config = choice - 1"
        done

        if [[ $choice -eq 0 ]]; then
            echo "Not using any config file."
            echo "gtkwave \"$1\""
            gtkwave "$1"
        else
            echo "Going with ${configs[$i_config]} ($choice)."
            echo "gtkwave -a \"${configs[$i_config]}\" \"$1\""
            gtkwave -a "${configs[$i_config]}" "$1"
        fi
    fi
else
    #No config 
    echo "No gtkwave configs found."
    echo "gtkwave \"$1\""
    gtkwave "$1"
fi
