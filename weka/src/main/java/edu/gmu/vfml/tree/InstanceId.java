package edu.gmu.vfml.tree;

import weka.core.Instance;

/**
 * <p>A wrapper class around a {@code weka.core.Instance} which automatically
 * assigns the instance a monotonically increasing identifier to each
 * instance created.</p>
 * 
 * <p>Note: This class is not thread safe.</p>
 * 
 * @see weka.classifiers.trees.CVFDT
 * @author ulman
 */
public class InstanceId
{
    private static long counter = 0;

    protected Instance instance;
    protected long id;

    public InstanceId( Instance instance )
    {
        this.instance = instance;
        this.id = counter++;
    }

    public Instance getInstance( )
    {
        return instance;
    }

    public long getId( )
    {
        return id;
    }
}
