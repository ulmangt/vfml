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
    protected Map<Attribute,CNode> altNodes;
    
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
        return (CNode) getLeafNode( this, instance );
    }
    
    @Override
    public CNode getSuccessor( int value )
    {
        if ( successors != null )
        {
            return (CNode) successors[value];
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
    
    /**
     * Called when enough data instances have been seen that it is time to end test mode.
     */
    protected void endTest( )
    {
        this.testCount = 0;
        this.testMode = false;
    }
    
    /**
     * Called when enough data instances have been seen that it is time to enter test mode.
     */
    protected void startTest( )
    {
        this.testCount = 0;
        this.testMode = true;
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
