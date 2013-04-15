#!/bin/bash

# download the airline data set
./download_data.sh

# run data formatting python script to make one large arff formatted file
python formatData.py . data header

# the arff header and data are written to two files which need to be combined
cat header data > FlightDelayData.arff
rm header data

