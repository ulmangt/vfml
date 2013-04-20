package edu.gmu.vfml.ui.painter;

import java.awt.Color;

import javax.media.opengl.GL;

import com.metsci.glimpse.axis.Axis2D;
import com.metsci.glimpse.context.GlimpseBounds;
import com.metsci.glimpse.painter.base.GlimpseDataPainter2D;
import com.metsci.glimpse.support.color.GlimpseColor;
import com.metsci.glimpse.support.font.FontUtils;
import com.sun.opengl.util.j2d.TextRenderer;

import edu.gmu.vfml.ui.VisualizableNode;

public class VisualizableNodePainter extends GlimpseDataPainter2D
{
    protected VisualizableNode node;
    protected TextRenderer textRenderer;
    
    public VisualizableNodePainter( )
    {
        this.textRenderer = new TextRenderer( FontUtils.getDefaultBold( 14.0f ) );
    }
    
    public void setVisualizableNode( VisualizableNode node )
    {
        this.node = node;
    }
    
    @Override
    public void paintTo( GL gl, GlimpseBounds bounds, Axis2D axis )
    {
        this.textRenderer.setColor( Color.black );
        GlimpseColor.glColor( gl, GlimpseColor.getBlack( ) );
        gl.glPointSize( 18f );
        gl.glLineWidth( 3.5f );
        
        if ( this.node != null )
        {
            paintNode( this.node, gl, bounds, axis );
        }
    }
    
    private void paintNode( VisualizableNode node, GL gl, GlimpseBounds bounds, Axis2D axis )
    {
        if  ( node.getText( ) != null )
        {
            this.textRenderer.beginRendering( bounds.getWidth( ), bounds.getHeight( ) );
            try
            {
                int x = axis.getAxisX( ).valueToScreenPixel( node.getX( ) ) + 10;
                int y = axis.getAxisY( ).valueToScreenPixel( node.getY( ) );
                
                this.textRenderer.draw( node.getText( ), x, y );
            }
            finally
            {
                this.textRenderer.endRendering( );
            }
        }
        
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