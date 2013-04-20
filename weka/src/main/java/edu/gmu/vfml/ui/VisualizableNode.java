package edu.gmu.vfml.ui;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import weka.core.Attribute;
import edu.gmu.vfml.tree.Node;

public class VisualizableNode
{
    protected String text = null;
    protected List<VisualizableNode> successors;
    
    protected float x;
    protected float y;
    
    public VisualizableNode( String text, List<VisualizableNode> successors )
    {
        this.text = text;
        this.successors = Collections.unmodifiableList( successors );
    }
    
    public float getX( )
    {
        return this.x;
    }
    
    public float getY( )
    {
        return this.y;
    }
    
    public String getText( )
    {
        return this.text;
    }
    
    public List<VisualizableNode> getSuccessors( )
    {
        return this.successors;
    }
    
    private static final void setPosition( VisualizableNode node )
    {
        setPosition0( node, 0, -1, 1 );
    }
    
    private static final void setPosition0( VisualizableNode node, int level, float left, float right )
    {
        float center = ( right + left ) / 2.0f;
        node.x = center;
        node.y = level;
        
        int size = node.getSuccessors( ).size( );
        float step = ( right - left ) / size;
        
        for ( int i = 0 ; i < size ; i++ )
        {
            float newLeft = left + step * i;
            float newRight = left + (step+1) * i;
            int newLevel = level-1;
            VisualizableNode successor = node.getSuccessors( ).get( i );
            setPosition0( successor, newLevel, newLeft, newRight );
        }
    }
    
    public static final VisualizableNode copyTree( Node node )
    {
        VisualizableNode vNode = copyTree0( node );
        setPosition( vNode );
        return vNode;
    }
    
    private static final VisualizableNode copyTree0( Node node )
    {
        List<VisualizableNode> successors = new LinkedList<VisualizableNode>( );
        String text = null;

        Attribute splitAttribute = node.getAttribute( );
        
        if ( splitAttribute != null )
        {
            int numValues = splitAttribute.numValues( );
            
            for ( int i  = 0 ; i < numValues ; i++ )
            {
                Node successor = node.getSuccessor( i );
                VisualizableNode vSuccessor = copyTree( successor );
                successors.add( vSuccessor );
                text = String.format( "%s = %s", splitAttribute.name( ), splitAttribute.value( i ) );
            }
        }
        else
        {
            String classValue = node.getClassAttribute( ).value( (int) node.getClassValue( ) );
            text = String.format( "class = %s", classValue );
        }
        
        return new VisualizableNode( text, successors );
    }
}
