package edu.gmu.vfml.data;

import java.util.Random;

import weka.core.Attribute;
import weka.core.FastVector;
import weka.core.Instance;
import weka.core.Instances;

public class RandomDataGenerator
{
    private int numAttributes;
    private double errorRate;
    private Instances instances;
    private BooleanConcept concept;
    private Random r;

    public RandomDataGenerator( BooleanConcept concept, int numAttributes, double errorRate )
    {
        this.concept = concept;
        this.numAttributes = numAttributes;
        this.errorRate = errorRate;
        this.r = new Random( );
        
        FastVector attributes = new FastVector( );
        for ( int i = 0 ; i < numAttributes ; i++ )
        {
            String attributeName = String.format( "v%s", i );
            attributes.addElement( new Attribute( attributeName, getAttributeValues() ) );
        }
        
        attributes.addElement( new Attribute( "class", getAttributeValues() ) );
        
        instances = new Instances( "data", attributes, 0 );
    }
    
    private static final FastVector getAttributeValues( )
    {
        FastVector attributeValues = new FastVector( );
        attributeValues.addElement( "0" );
        attributeValues.addElement( "1" );
        return attributeValues;
    }

    public Instance next( )
    {
        boolean[] values = new boolean[numAttributes];
        double[] wekaValues = new double[numAttributes+1];
        
        for ( int i = 0 ; i < numAttributes ; i++ )
        {
            values[i] = r.nextBoolean( );
            wekaValues[i] = values[i] ? 1 : 0;
        }
        
        boolean classValue = concept.f( values );
        
        if ( r.nextDouble( ) < errorRate )
        {
            classValue = !classValue;
        }
        
        wekaValues[numAttributes] = classValue ? 1 : 0;
        
        Instance instance = new Instance( 1.0, wekaValues );
        instance.setDataset( instances );
        
        return instance;
    }
}
