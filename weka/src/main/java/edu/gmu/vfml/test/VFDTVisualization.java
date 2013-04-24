package edu.gmu.vfml.test;

import com.metsci.glimpse.examples.Example;
import com.metsci.glimpse.support.repaint.RepaintManager;

import weka.classifiers.trees.VFDT;
import weka.core.Instance;
import weka.core.NoSupportForMissingValuesException;
import edu.gmu.vfml.data.BooleanConcept;
import edu.gmu.vfml.data.RandomDataGenerator;
import edu.gmu.vfml.ui.TreeVisualization;
import edu.gmu.vfml.ui.VisualizableNode;

public class VFDTVisualization
{
    public static void main( String[] args ) throws Exception
    {
    	new VFDTVisualization( ).run( );
    }
	
    public void run( ) throws Exception
    {
        boolean conceptFlag = true;
        
        // define a boolean concept
        BooleanConcept concept1 = new BooleanConcept( )
        {
            @Override
            public boolean f( boolean[] v )
            {
                return ( v[1] && ( v[5] || v[6] || !v[12] ) && !v[7] ) || ( v[3] && ( !v[2] || v[4] ) && v[12] ) || ( v[6] && v[12] );
            }
        };

        // define another boolean concept
        BooleanConcept concept2 = new BooleanConcept( )
        {
            @Override
            public boolean f( boolean[] v )
            {
                return ( v[1] && ( !v[5] || v[6] || !v[12] ) && v[7] ) || ( v[3] && v[4] && v[12] ) || ( v[6] && v[12] );
            }
        };
        
        // build a random data generator using the concept
        // (which randomly flips the class value 5% of the time)
        RandomDataGenerator generator = new RandomDataGenerator( concept1, 15, 0.05 );

        // build a CVFDT classifier
        final VFDT classifier = buildClassifier( generator );

        final TreeVisualization visualization = new TreeVisualization( );
        final Example example = Example.showWithSwing( visualization );
        final RepaintManager repaintManager = example.getManager( );
        
        for ( int i = 0;; i++ )
        {
            Instance instance = generator.next( );
            classifier.addInstance( instance );

            // periodically switch the concept to test concept drift adaptation
            if ( i % 200000 == 0 )
            {
                if ( conceptFlag ) generator.setConcept( concept2 );
                else generator.setConcept( concept1 );
                
                conceptFlag = !conceptFlag;
            }
            
            if ( i % 5000 == 0 )
            {
                System.out.println( i );
            }
            
            if ( i % 1000 == 0 )
            {
                // make a copy of the current classifier tree structure
                final VisualizableNode vNode = VisualizableNode.copyTree( classifier.getRoot( ) );
                
                final double accuracy = testAccuracy( classifier, generator );
                final int n = i;
                
                repaintManager.asyncExec( new Runnable( )
                {
                    @Override
                    public void run( )
                    {
                        visualization.setNode( vNode );
                        visualization.addAccuracy( n, accuracy );
                    }
                });
            }
        }
    }
    
    protected VFDT buildClassifier( RandomDataGenerator generator ) throws Exception
    {
        // build a VFDT classifier
        final VFDT classifier = new VFDT( );
        classifier.setConfidenceLevel( 1e-6 );
        classifier.setNMin( 30 );
        classifier.initialize( generator.getDataset( ) );
        
        return classifier;
    }
    
    protected double testAccuracy( VFDT classifier, RandomDataGenerator generator ) throws NoSupportForMissingValuesException
    {
    	int total = 500;
    	int correct = 0;
    	
    	int classIndex = generator.getDataset().classIndex();
    	
    	for ( int i = 0 ; i < total ; i++ )
    	{
    		Instance instance = generator.next();
    		double result = classifier.classifyInstance( instance );
    		if ( instance.value( classIndex ) == result )
    		{
    			correct++;
    		}
    	}
    	
    	return (double) correct / (double) total;
    }
}
