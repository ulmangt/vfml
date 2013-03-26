package edu.gmu.vfml.tree;

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
}
