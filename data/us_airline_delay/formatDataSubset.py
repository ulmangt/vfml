import csv
from os import listdir
from os.path import isfile, join
import sys

num_attributes = 29

# keep track of the values for the nominal
# attributes in the data set
nom_carrier = set()
nom_origin = set()
nom_dest = set()

# first argument to the script should be the directory to process
directory = sys.argv[1]

# second argument to the script is an output file
out_file_name = sys.argv[2]
out_file = open( out_file_name, 'w' )
out_file.write( '@DATA\n' )

# third argument is a header file (arff header data will be output here)
header_file_name = sys.argv[3]
header_file = open( header_file_name, 'w' )

# build a list of csv files in the directory
files = [ f for f in listdir( directory ) if isfile( join( directory,f ) ) and f.endswith( '.csv' ) ]

# loop over the csv files in the directory
for file_name in files:
    print 'Opening:', file_name
    file_handle = open( join( directory,file_name ), 'r' )

    # read each file line by line
    count = 0
    for line in file_handle:
        count = count + 1

        # periodically report on progress
        if count % 10000 == 0:
            print file_name, count

        try:
            # split the line by commas
            tokens = line.split(',')
        
            # skip lines with the wrong number of attributes
            if len(tokens) != num_attributes:
                continue

            # replace NA or blank attributes with ?
            for i in xrange(num_attributes):
                # remove all leading/trailing whitespace and
                # remove inner spaces (which mess up weka arff parser)
                token = tokens[i].strip().replace(' ','')
                if token == 'NA' or token == '':
                    tokens[i] = '?'
                else:
                    tokens[i] = token

            if count > 1 :
                # store unique nominal values
                nom_carrier.add( tokens[8] )
                nom_origin.add( tokens[16] )
                nom_dest.add( tokens[17] )

                # write line back to output file
                part1 = ','.join(tokens[0:9])
                part2 = ','.join(tokens[11:14])
                part3 = ','.join(tokens[15:21])

                # create synthetic attribute
                # (whether plane's arrival was delayed)
                if tokens[14] == '?':
                    delayed = '?'
                else:
                    arrivalDelay = int(tokens[14])
                    if arrivalDelay > 0:
                        delayed = 'yes'
                    else:
                        delayed = 'no'

                out_file.write(part1+','+part2+','+part3+','+delayed+'\n')
        except:
            pass

    file_handle.close()

if '?' in nom_carrier:
    nom_carrier.remove( '?' )

if '?' in nom_origin:
    nom_origin.remove( '?' )

if '?' in nom_dest:
    nom_dest.remove( '?' )

# write the arff header information
header_file.write( '@RELATION FlightDelay\n' )
header_file.write( '@ATTRIBUTE Year NUMERIC\n' )
header_file.write( '@ATTRIBUTE Month NUMERIC\n' )
header_file.write( '@ATTRIBUTE DayofMonth NUMERIC\n' )
header_file.write( '@ATTRIBUTE DayOfWeek NUMERIC\n' )
header_file.write( '@ATTRIBUTE DepTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE CRSDepTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE ArrTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE CRSArrTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE UniqueCarrier ' + '{' + ','.join(nom_carrier) + '}\n' )
header_file.write( '@ATTRIBUTE ActualElapsedTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE CRSElapsedTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE AirTime NUMERIC\n' )
header_file.write( '@ATTRIBUTE DepDelay NUMERIC\n' )
header_file.write( '@ATTRIBUTE Origin ' + '{' + ','.join(nom_origin) + '}\n' )
header_file.write( '@ATTRIBUTE Dest ' + '{' + ','.join(nom_dest) + '}\n' )
header_file.write( '@ATTRIBUTE Distance NUMERIC\n' )
header_file.write( '@ATTRIBUTE TaxiIn NUMERIC\n' )
header_file.write( '@ATTRIBUTE TaxiOut NUMERIC\n' )
header_file.write( '@ATTRIBUTE ArrivalDelayed {yes,no}\n' )

# close output files
header_file.flush()
header_file.close()
out_file.flush()
out_file.close()
