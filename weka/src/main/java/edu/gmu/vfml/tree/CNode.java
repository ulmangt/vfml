package edu.gmu.vfml.tree;

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

    public CNode( Attribute[] attributes, Attribute classAttribute, int id )
    {
        super( attributes, classAttribute );
        this.id = id;
    }

    public CNode( Instances instances, Attribute classAttribute, int id )
    {
        super( instances, classAttribute );
        this.id = id;
    }

    public CNode( Instance instance, Attribute classAttribute, int id )
    {
        super( instance, classAttribute );
        this.id = id;
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
    
    /**
     * Called when an instance rolls off the window. Removes the instance
     * from the counts of each node
     */
    public void traverseAndDecrementCounts( Instance instance, int id )
    {
        // nodes with greater id than the instance id were created after the
        // instance arrived and do not have the instance data included in their counts
        if ( this.id <= id )
        {
            decrementCounts( instance );
        }
        
        // traverse into all the alternative nodes
        for ( CNode alt : altNodes.values( ) )
        {
            alt.traverseAndDecrementCounts( instance, id );
        }
        
        // if the main tree node is not a leaf node,
        // descend into the appropriate child node
        if ( getAttribute( ) != null )
        {
            int attributeValue = ( int ) instance.value( getAttribute( ) );
            CNode childNode = getSuccessor( attributeValue );
            childNode.traverseAndDecrementCounts( instance, id );
        }
    }
    
    /**
     * In addition to incrementing the counts for the main tree,
     * increment the counts of any alternative trees being grown from this Node.
     */
    public void traverseAndIncrementCounts( Instance instance )
    {
        // increment the counts for this node
        // (unlike VFDT, statistics are kept for each data instance
        // at every node in the tree in order to continuously monitor
        // the validity of previous decisions)
        incrementCounts( instance );
        
        // traverse into all the alternative nodes
        for ( CNode alt : altNodes.values( ) )
        {
            alt.traverseAndIncrementCounts( instance );
        }
        
        // if the main tree node is not a leaf node,
        // descend into the appropriate child node
        if ( getAttribute( ) != null )
        {
            int attributeValue = ( int ) instance.value( getAttribute( ) );
            CNode childNode = getSuccessor( attributeValue );
            childNode.traverseAndIncrementCounts( instance );
        }
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
            this.successors[valueIndex] = new CNode( instance, classAttribute, id );
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
