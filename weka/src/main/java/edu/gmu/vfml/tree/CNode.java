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
    protected Map<Attribute,Node> altNodes;

    public CNode( Attribute[] attributes, Attribute classAttribute )
    {
        super( attributes, classAttribute );
    }

    public CNode( Instances instances, Attribute classAttribute )
    {
        super( instances, classAttribute );
    }

    public CNode( Instance instance, Attribute classAttribute )
    {
        super( instance, classAttribute );
    }
    
    /**
     * In addition to incrementing the counts for the main tree,
     * increment the counts of any alternative trees being grown from this Node.
     */
    @Override
    public void incrementCounts( Instance instance )
    {
        super.incrementCounts( instance );
        
        for ( Node alt : altNodes.values( ) )
        {
            alt.incrementCounts( instance );
        }
    }
}
