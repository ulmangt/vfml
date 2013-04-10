#!/bin/bash

# Downloads 12GB (after decompression) of flight delay data (123 million rows)
# Data is hosted by the ASA Statistical Computing and Statistical Graphics sections
# Information on data fields is available at: http://stat-computing.org/dataexpo/2009/the-data.html

for year in {1987..2008}
do
    # download a year of data
    wget "http://stat-computing.org/dataexpo/2009/${year}.csv.bz2"
    
    # decompress bz2 compressed data
    bunzip2 "${year}.csv.bz2"
done

# run data formatting python script to make one large arff formatted file
python formatData.py . data.arff header.arff

# the arff header and data are written to two files which need to be combined
cat header.arff data.arff > FlightDelay.arff
rm header.arff data.arff
