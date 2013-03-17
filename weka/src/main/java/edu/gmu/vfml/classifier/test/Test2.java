package edu.gmu.vfml.classifier.test;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

import weka.core.Instances;
import weka.filters.Filter;
import weka.filters.unsupervised.attribute.ReplaceMissingValues;

import com.metsci.glimpse.util.io.StreamOpener;

import edu.gmu.vfml.classifier.VFDT;

public class Test2
{
    public static void main( String[] args ) throws Exception
    {
        // read data set
        InputStream in = StreamOpener.fileThenResource.openForRead( "data/breast-cancer.arff" );
        BufferedReader reader = new BufferedReader( new InputStreamReader( in ) );
        Instances data = new Instances( reader );
        reader.close( );

        // set class index as last attribute in data set (arff default)
        data.setClassIndex( data.numAttributes( ) - 1 );

        // replace missing values with modes from the data set
        data = Filter.useFilter( data, new ReplaceMissingValues( ) );
        
        // build a VFDT classifier and apply it to the data
        VFDT classifier = new VFDT( );
        classifier.buildClassifier( data );
    }
}
