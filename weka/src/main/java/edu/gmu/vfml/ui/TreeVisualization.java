package edu.gmu.vfml.ui;

import com.metsci.glimpse.axis.Axis1D;
import com.metsci.glimpse.axis.Axis2D;
import com.metsci.glimpse.axis.listener.mouse.AxisMouseListener2D;
import com.metsci.glimpse.layout.GlimpseAxisLayout2D;
import com.metsci.glimpse.layout.GlimpseLayout;
import com.metsci.glimpse.layout.GlimpseLayoutProvider;
import com.metsci.glimpse.painter.decoration.BackgroundPainter;
import com.metsci.glimpse.support.color.GlimpseColor;

import edu.gmu.vfml.ui.painter.VisualizableNodePainter;

public class TreeVisualization implements GlimpseLayoutProvider
{
    VisualizableNodePainter treePainter;

    @Override
    public GlimpseLayout getLayout( ) throws Exception
    {
        // create an Axis2D to define the bounds of the plotting area
        Axis2D axis = new Axis2D( );

        double minX = -0.8;
        double maxX = 0.8;
        double minY = -8.0;
        double maxY = 0.0;

        // set the axis bounds (display all the data initially)
        axis.set( minX, maxX, minY, maxY );

        // create a layout to draw the geographic data
        GlimpseAxisLayout2D layout = new GlimpseAxisLayout2D( "TreeVisualization", axis );

        // add an axis mouse listener to the layout so that it responds to user pans/zooms
        layout.addGlimpseMouseAllListener( new AxisMouseListener2D( )
        {
            // double horizontal zoom factor
            public void zoom( Axis1D axis, boolean horizontal, int zoomIncrements, int posX, int posY )
            {
                if ( horizontal ) super.zoom( axis, horizontal, zoomIncrements*2, posX, posY );
                else super.zoom( axis, horizontal, zoomIncrements, posX, posY );
            }
        } );

        // add a solid color background painter to the plot
        layout.addPainter( new BackgroundPainter( ).setColor( GlimpseColor.getWhite( ) ) );

        // add grid lines to the plot
        //layout.addPainter( new GridPainter( ).setLineColor( GlimpseColor.getGray( 0.7f ) ).setShowMinorGrid( true ) );

        // add a painter to display the tree structure
        treePainter = new VisualizableNodePainter( );
        layout.addPainter( treePainter );

        return layout;
    }

    public void setNode( VisualizableNode node )
    {
        this.treePainter.setVisualizableNode( node );
    }
}