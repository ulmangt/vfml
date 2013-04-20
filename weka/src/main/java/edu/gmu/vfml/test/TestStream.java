package edu.gmu.vfml.test;

import weka.core.Instance;
import edu.gmu.vfml.data.BooleanConcept;
import edu.gmu.vfml.data.RandomDataGenerator;

public class TestStream
{
    public static void main( String[] args )
    {
        BooleanConcept concept = new BooleanConcept( )
        {
            @Override
            public boolean f( boolean[] v )
            {
                return ( v[1] && (v[5] || v[6] || !v[12]) && !v[7] ) || ( v[3] && (!v[2] || v[4]) && v[12] ) || ( v[6] && v[12] );
            }
        };
        
        RandomDataGenerator generator = new RandomDataGenerator( concept, 15, 0.05 );
        
        while ( true )
        {
            Instance instance = generator.next( );
            System.out.println( instance );
        }
    }
}
