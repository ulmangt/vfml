package edu.gmu.vfml.test;

import java.io.File;

import weka.core.Instances;
import weka.core.converters.C45Loader;
import edu.gmu.vfml.classifier.VFDT;

public class Test3
{
    public static void main( String[] args ) throws Exception
    {
        C45Loader loader = new C45Loader( );
        ClassLoader classloader = Test3.class.getClassLoader( );
        File file = new File( classloader.getResource( "data/binary.names" ).getFile( ) );
        loader.setSource( file );
        Instances data = loader.getDataSet( );

        // set class index as last attribute in data set (arff default)
        data.setClassIndex( data.numAttributes( ) - 1 );

        // build a VFDT classifier
        VFDT classifier = new VFDT( );
        // set a very low confidence level so that not much data is needed for each split
        classifier.setConfidenceLevel( 1e-1 );
        // apply the classifier to the data set
        classifier.buildClassifier( data );
    }
}
