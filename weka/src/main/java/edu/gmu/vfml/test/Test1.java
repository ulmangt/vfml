package edu.gmu.vfml.test;

import weka.classifiers.Classifier;
import weka.classifiers.trees.VFDT;
import weka.core.Attribute;
import weka.core.FastVector;
import weka.core.Instance;
import weka.core.Instances;

public class Test1
{
    public static void main( String[] args ) throws Exception
    {
        // build a list of attributes for each instance in the data set
        FastVector attributes = new FastVector( 2 );

        FastVector classValues = new FastVector( 2 );
        classValues.addElement( "yes" );
        classValues.addElement( "no" );
        attributes.addElement( new Attribute( "Class", classValues ) );

        attributes.addElement( new Attribute( "Numeric Attribute 1" ) );

        FastVector attribute2Values = new FastVector( 2 );
        classValues.addElement( "blue" );
        classValues.addElement( "green" );
        classValues.addElement( "yellow" );
        attributes.addElement( new Attribute( "Nominal Attribute 2", attribute2Values ) );

        // create a new data set
        int initalCapacity = 100;
        Instances data = new Instances( "Test Dataset 1", attributes, initalCapacity );

        // create an instance and add it to the data set
        Instance instance1 = new Instance( 3 );

        Attribute attributeClass = data.attribute( "Class" );
        instance1.setValue( attributeClass, "no" );

        Attribute attribute1 = data.attribute( "Numeric Attribute 1" );
        instance1.setValue( attribute1, 2.0 );

        Attribute attribute2 = data.attribute( "Nominal Attribute 2" );
        instance1.setValue( attribute2, "green" );
        
        data.add( instance1 );

        // build a VFML classifier using the data set
        Classifier classifier = new VFDT( );
        classifier.buildClassifier( data );
    }
}
