package edu.gmu.vfml.ui;

import com.metsci.glimpse.axis.Axis2D;
import com.metsci.glimpse.axis.listener.mouse.AxisMouseListener2D;
import com.metsci.glimpse.layout.GlimpseAxisLayout2D;
import com.metsci.glimpse.layout.GlimpseLayout;
import com.metsci.glimpse.layout.GlimpseLayoutProvider;
import com.metsci.glimpse.painter.decoration.BackgroundPainter;
import com.metsci.glimpse.painter.decoration.GridPainter;
import com.metsci.glimpse.support.color.GlimpseColor;

public class TreeVisualization implements GlimpseLayoutProvider
{
    @Override
    public GlimpseLayout getLayout( ) throws Exception
    {
        // create an Axis2D to define the bounds of the plotting area
        Axis2D axis = new Axis2D( );

        double minX = 0.0;
        double maxX = 1.0;
        double minY = 0.0;
        double maxY = 1.0;
        
        // set the axis bounds (display all the data initially)
        axis.set( minX, maxX, minY, maxY );
        axis.lockAspectRatioXY( 1.0 );

        // create a layout to draw the geographic data
        GlimpseAxisLayout2D layout = new GlimpseAxisLayout2D( "TreeVisualization", axis );

        // add an axis mouse listener to the layout so that it responds to user pans/zooms
        layout.addGlimpseMouseAllListener( new AxisMouseListener2D( ) );

        // add a solid color background painter to the plot
        layout.addPainter( new BackgroundPainter( ).setColor( GlimpseColor.getBlack( ) ) );

        // add grid lines to the plot
        layout.addPainter( new GridPainter( ).setLineColor( GlimpseColor.getGray( 0.7f ) ).setShowMinorGrid( true ) );

        return layout;
    }
}