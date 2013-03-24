package edu.gmu.vfml.tree;

import java.io.Serializable;

import weka.core.Attribute;
import weka.core.Instance;
import weka.core.Instances;

/**
 * <p>VFDT does not store entire training instances, only sufficient statistics
 * necessary to calculate Hoeffding bound and decide when to split nodes
 * and on which attributes to make the split.</p>
 * 
 * <p>Counts stores per-Node count values (in lieu of storing the entire set
 * of instances used at each Node.</p>
 */
public class Node implements Serializable
{
    private static final long serialVersionUID = 1L;

    /** The node's successors. */
    private Node[] successors;

    /** Attribute used for splitting. */
    private Attribute attribute;

    /** Class value if node is leaf. */
    private double classValue;

    /** Number of instances corresponding to classValue.
     *  This is equal to classCounts[classAttribute.index()]. */
    private int classCount;

    // fields copied from VFDT
    Attribute classAttribute;

    // counts indexed by [attribute][value][class]
    transient int[][][] counts;
    transient int[] classCounts;
    transient int totalCount;

    public Node( Instances instance, Attribute classAttribute )
    {
        this.classAttribute = classAttribute;

        this.classCounts = new int[classAttribute.numValues( )];
        this.counts = new int[instance.numAttributes( )][][];
        for ( int i = 0; i < instance.numAttributes( ); i++ )
        {
            Attribute attribute = instance.attribute( i );
            int[][] attributeCounts = new int[attribute.numValues( )][];
            this.counts[i] = attributeCounts;

            for ( int j = 0; j < attribute.numValues( ); j++ )
            {
                attributeCounts[j] = new int[classAttribute.numValues( )];
            }
        }
    }

    public Node( Instance instance, Attribute classAttribute )
    {
        this.classAttribute = classAttribute;

        this.classCounts = new int[classAttribute.numValues( )];
        this.counts = new int[instance.numAttributes( )][][];
        for ( int i = 0; i < instance.numAttributes( ); i++ )
        {
            Attribute attribute = instance.attribute( i );
            int[][] attributeCounts = new int[attribute.numValues( )][];
            this.counts[i] = attributeCounts;

            for ( int j = 0; j < attribute.numValues( ); j++ )
            {
                attributeCounts[j] = new int[classAttribute.numValues( )];
            }
        }
    }
    
    public Node getSuccessor( int value )
    {
        if ( successors != null )
        {
            return successors[value];
        }
        else
        {
            return null;
        }
    }
    
    public Attribute getClassAttribute( )
    {
        return classAttribute;
    }
    
    public Attribute getAttribute( )
    {
        return attribute;
    }
    
    public double getClassValue( )
    {
        return classValue;
    }
    
    public void split( Attribute attribute, Instance instance )
    {
        this.successors = new Node[attribute.numValues( )];
        this.attribute = attribute;

        for ( int valueIndex = 0; valueIndex < attribute.numValues( ); valueIndex++ )
        {
            this.successors[valueIndex] = new Node( instance, classAttribute );
        }
    }

    public void incrementCounts( Instance instance )
    {
        int instanceClassValue = ( int ) instance.classValue( );

        incrementTotalCount( );
        incrementClassCount( instanceClassValue );

        for ( int i = 0; i < instance.numAttributes( ); i++ )
        {
            Attribute attribute = instance.attribute( i );
            incrementCount( attribute, instance );
        }

        // update classValue and classCount
        int instanceClassCount = getCount( instanceClassValue );

        // if the count of the class we just added is greater than the current
        // largest count, it becomes the new classification for this node
        if ( instanceClassCount > classCount )
        {
            classCount = instanceClassCount;
            classValue = instance.value( classAttribute );
        }
    }

    /**
     * @return the total number of instances in this Node
     */
    public int getCount( )
    {
        return totalCount;
    }

    /**
     * @param classIndex the class to get counts for
     * @return the total number of instances for the provided class
     */
    public int getCount( int classIndex )
    {
        return classCounts[classIndex];
    }

    /**
    * @param attribute the attribute to get a count for
    * @param valueIndex the value of the attribute
    * @return the total number of instances with the provided attribute value
    */
    public int getCount( Attribute attribute, int valueIndex )
    {
        int attributeIndex = attribute.index( );

        int count = 0;
        for ( int classIndex = 0; classIndex < classAttribute.numValues( ); classIndex++ )
        {
            count += getCount( attributeIndex, valueIndex, classIndex );
        }

        return count;
    }

    public int getCount( Attribute attribute, int valueIndex, int classIndex )
    {
        return getCount( attribute.index( ), valueIndex, classIndex );
    }

    /**
     * @param classIndex
     * @param attributeIndex
     * @param valueIndex
     * @return the number of instances with the provided class and attribute value
     */
    public int getCount( int attributeIndex, int valueIndex, int classIndex )
    {
        return counts[attributeIndex][valueIndex][classIndex];
    }

    private void incrementTotalCount( )
    {
        totalCount += 1;
    }

    private void incrementClassCount( int classIndex )
    {
        classCounts[classIndex] += 1;
    }

    private void incrementCount( Attribute attribute, Instance instance )
    {
        int attributeIndex = attribute.index( );
        int classValue = ( int ) instance.value( classAttribute );
        int attributeValue = ( int ) instance.value( attribute );
        incrementCount( attributeIndex, attributeValue, classValue );
    }

    private void incrementCount( int attributeIndex, int valueIndex, int classIndex )
    {
        counts[attributeIndex][valueIndex][classIndex]++;
    }
}