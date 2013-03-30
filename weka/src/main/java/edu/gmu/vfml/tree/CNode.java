package edu.gmu.vfml.tree;

import java.util.Collection;
import java.util.Map;

import weka.core.Attribute;
import weka.core.Instance;
import weka.core.Instances;

/**
 * <p>A helper class for {@code weka.classifiers.trees.CVFDT}. Stores the nested
 * tree structure of attribute splits and associated Node statistics.</p>
 * 
 * @author ulman
 */
public class CNode extends Node
{
    private static final long serialVersionUID = 1L;

    /**
     * A map of subtrees made from splitting on alternative Attributes (instead of
     * splitting on the Attribute specified in this.attribute).
     */
    protected Map<Attribute, CNode> altNodes;

    /**
     * @see InstanceId
     */
    protected int id;

    protected int testInterval;
    protected int testDuration;

    /**
     * Number of instances until entering/exiting next test phase.
     */
    transient protected int testCount = 0;

    /**
     * If true, new data instances are not used to grow the tree. Instead, they are
     * used to compare the error rate of this Node to that of its subtrees.
     */
    transient protected boolean testMode = false;

    /**
     * The number of correctly classified test instances.
     */
    transient protected int testCorrectCount = 0;

    public CNode( Attribute[] attributes, Attribute classAttribute, int id, int testInterval, int testDuration )
    {
        super( attributes, classAttribute );
        this.id = id;
    }

    public CNode( Instances instances, Attribute classAttribute, int id, int testInterval, int testDuration )
    {
        super( instances, classAttribute );
        this.id = id;
    }

    public CNode( Instance instance, Attribute classAttribute, int id, int testInterval, int testDuration )
    {
        super( instance, classAttribute );
        this.id = id;
    }

    @Override
    public CNode getLeafNode( Instance instance )
    {
        return ( CNode ) getLeafNode( this, instance );
    }

    @Override
    public CNode getSuccessor( int value )
    {
        if ( successors != null )
        {
            return ( CNode ) successors[value];
        }
        else
        {
            return null;
        }
    }

    public Collection<CNode> getAlternativeTrees( )
    {
        return altNodes.values( );
    }

    /**
     * Determines the class prediction of the tree rooted at this node for the given instance.
     * Compares that prediction against the true class value, and if they are equal, increments
     * the correct test counter for this node.
     * 
     * @param instance
     */
    public void testInstance( Instance instance )
    {
        double predicted = getLeafNode( instance ).getClassValue( );
        double actual = instance.classValue( );
        if ( predicted == actual )
        {
            this.testCorrectCount++;
        }
    }

    public void incrementTestCount( )
    {
        // check whether we should enter or exit test mode
        this.testCount++;
        if ( this.testMode )
        {
            if ( this.testCount > this.testDuration )
            {
                endTest( );
            }
        }
        else
        {
            if ( this.testCount > this.testInterval )
            {
                startTest( );
            }
        }
    }

    public boolean isTestMode( )
    {
        return this.testMode;
    }
    
    public double getTestError( )
    {
        return 1 - (double) this.testCorrectCount / (double) this.testCount;
    }

    /**
     * Called when enough data instances have been seen that it is time to end test mode.
     */
    protected void endTest( )
    {
        CNode bestAlt = null;
        double bestError = getTestError( );
        for ( CNode alt : getAlternativeTrees( ) )
        {
            double error = alt.getTestError( );
            if ( error < bestError )
            {
                bestError = error;
                bestAlt = alt;
            }
        }
        
        // one of the alternative trees is better than the current tree!
        // replace this node with the alternative node
        if ( bestAlt != null )
        {
            this.successors = bestAlt.successors;
            this.attribute = bestAlt.attribute;
            this.classValue = bestAlt.classValue;
            this.classCount = bestAlt.classCount;
            this.counts = bestAlt.counts;
            this.classCount = bestAlt.classCount;
            this.totalCount = bestAlt.totalCount;
            this.id = bestAlt.id;
            this.altNodes = bestAlt.altNodes;
        }
        
        this.testCorrectCount = 0;
        this.testCount = 0;
        this.testMode = false;
    }

    /**
     * Called when enough data instances have been seen that it is time to enter test mode.
     */
    protected void startTest( )
    {
        this.testCorrectCount = 0;
        this.testCount = 0;
        
        // if there are no alternative nodes to test, don't enter test mode (wait another
        // testInterval instances then check again)
        this.testMode = !this.altNodes.isEmpty( );
    }

    /**
     * Like {@code Node#split(Attribute, Instance)}, but creates CNodes and
     * assigns the specified id to the Node.
     */
    public void split( Attribute attribute, Instance instance, int id )
    {
        this.successors = new CNode[attribute.numValues( )];
        this.attribute = attribute;

        for ( int valueIndex = 0; valueIndex < attribute.numValues( ); valueIndex++ )
        {
            this.successors[valueIndex] = new CNode( instance, classAttribute, id, testInterval, testDuration );
        }
    }

    /**
     * @see InstanceId
     */
    public int getId( )
    {
        return id;
    }
}
