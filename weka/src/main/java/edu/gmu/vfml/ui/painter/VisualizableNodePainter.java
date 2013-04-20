package edu.gmu.vfml.ui.painter;

import javax.media.opengl.GL;

import com.metsci.glimpse.axis.Axis2D;
import com.metsci.glimpse.context.GlimpseBounds;
import com.metsci.glimpse.painter.base.GlimpseDataPainter2D;

import edu.gmu.vfml.ui.VisualizableNode;

public class VisualizableNodePainter extends GlimpseDataPainter2D
{
    protected VisualizableNode node;
    
    public VisualizableNodePainter( )
    {
        
    }
    
    public void setVisualizableNode( VisualizableNode node )
    {
        this.node = node;
    }
    
    @Override
    public void paintTo( GL gl, GlimpseBounds bounds, Axis2D axis )
    {
        gl.glPointSize( 10f );
        gl.glLineWidth( 5.0f );
        
        if ( this.node != null )
        {
            paintNode( this.node, gl, bounds, axis );
        }
    }
    
    private void paintNode( VisualizableNode node, GL gl, GlimpseBounds bounds, Axis2D axis )
    {
        gl.glBegin( GL.GL_POINTS );
        try
        {
            gl.glVertex2f( node.getX( ), node.getY( ) );
        }
        finally
        {
            gl.glEnd( );
        }
        
        for ( VisualizableNode child : node.getSuccessors( ) )
        {
            gl.glBegin( GL.GL_LINES );
            try
            {
                gl.glVertex2f( node.getX( ), node.getY( ) );
                gl.glVertex2f( child.getX( ), child.getY( ) );
            }
            finally
            {
                gl.glEnd( );
            }
            
            paintNode( child, gl, bounds, axis );
        }
    }
}
