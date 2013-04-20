package edu.gmu.vfml.ui;

import java.util.LinkedList;
import java.util.List;

import weka.core.Attribute;
import edu.gmu.vfml.tree.Node;

public class VisualizableNode
{
    protected String text = null;
    protected List<VisualizableNode> successors;
    
    public VisualizableNode( String text, List<VisualizableNode> successors )
    {
        this.text = text;
        this.successors = successors;
    }
    
    public String getText( )
    {
        return this.text;
    }
    
    public static final VisualizableNode copyTree( Node node )
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
