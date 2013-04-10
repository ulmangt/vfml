US Airline Delay
====

Flight arival and departure data for 120 million US flights from October 1987 through 2008. The dataset is 12 GB uncompressed.

The data is provided by the [Research and Innovative Technology Administration](http://www.transtats.bts.gov/OT_Delay/OT_DelayCause1.asp) which is a branch of the Bureau of Transportation Statistics. It was used as part of the 2009 American Statistical Association Data Expo and the data is also available from the [ASA website](http://stat-computing.org/dataexpo/2009/the-data.html).

Scripts
=====

#### downloadAndFormatData.sh

Master script which invokes both the *downloadData.sh* and *formatData.py* scripts to build a *FlightDelayData.arff* file containing the entire data set. This file is suitable for feeding to [Weka](http://www.cs.waikato.ac.nz/ml/weka/) or [Moa](http://moa.cms.waikato.ac.nz/) for classificaion.

#### downloadData.sh

Linux bash shell script which downloads 1.6 GB of data from the [ASA website](http://stat-computing.org/dataexpo/2009/the-data.html) as 22 bzipped csv files. The script also automatically uncompresses the files. The script takes no arguments.

#### formatData.py

Takes the set of CSV files downloaded by the *downloadData.sh* script produces an [ARFF](http://www.cs.waikato.ac.nz/ml/weka/arff.html) formatted version of the data.

*Usage:* `python formatData.py <path_to_data_directory> <data_output_file> <header_output_file>`

#### formatDataSubset.py

Similar to *formatData.py*, except a number of attributes are ignorred in order to focus on predicting the arrival delay of the flight.
