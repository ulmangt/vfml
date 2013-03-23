package edu.gmu.vfml.test;

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
        ReplaceMissingValues filter = new ReplaceMissingValues( );
        filter.setInputFormat( data );
        data = Filter.useFilter( data, filter );
        
        // build a VFDT classifier
        VFDT classifier = new VFDT( );
        // set a very low confidence level so that not much data is needed for each split
        classifier.setConfidenceLevel( 0.1 );
        // apply the classifier to the data set
        classifier.buildClassifier( data );
    }
}
