package edu.gmu.vfml.ui;

import com.metsci.glimpse.axis.Axis1D;
import com.metsci.glimpse.axis.Axis2D;
import com.metsci.glimpse.axis.listener.mouse.AxisMouseListener2D;
import com.metsci.glimpse.layout.GlimpseAxisLayout2D;
import com.metsci.glimpse.layout.GlimpseLayout;
import com.metsci.glimpse.layout.GlimpseLayoutManagerMig;
import com.metsci.glimpse.layout.GlimpseLayoutProvider;
import com.metsci.glimpse.painter.decoration.BackgroundPainter;
import com.metsci.glimpse.painter.decoration.BorderPainter;
import com.metsci.glimpse.painter.track.TrackPainter;
import com.metsci.glimpse.plot.SimplePlot2D;
import com.metsci.glimpse.support.color.GlimpseColor;

import edu.gmu.vfml.ui.painter.VisualizableNodePainter;

public class TreeVisualization implements GlimpseLayoutProvider
{
    VisualizableNodePainter treePainter;
    TrackPainter accuracyPainter;
    SimplePlot2D accuracyPlot;

    @Override
    public GlimpseLayout getLayout( ) throws Exception
    {
    	GlimpseLayout parent = new GlimpseLayout( );
    	
    	parent.setLayoutManager( new GlimpseLayoutManagerMig( "insets 4 4 4 4", "", "" ) );
    	
    	parent.addPainter( new BackgroundPainter( false ) );
    	
    	accuracyPlot = new SimplePlot2D( );
    	accuracyPlot.getAxis().getAxisY().setAbsoluteMin( 0.0 );
    	accuracyPlot.getAxis().getAxisY().setAbsoluteMax( 1.0 );
    	
    	accuracyPlot.getAxis().getAxisX().setMin( 0.0 );
    	accuracyPlot.getAxis().getAxisX().setMax( 1000 );
    	accuracyPlot.getAxis().getAxisX().setAbsoluteMin( 0.0 );
    	accuracyPlot.getAxis().getAxisX().setAbsoluteMax( 1000 );
    	
    	accuracyPlot.setTickSpacingY( 35 );
    	
    	accuracyPlot.setTitleHeight( 0 );
    	accuracyPlot.setBorderSize( 4 );
    	accuracyPlot.setAxisLabelX( "Number of Training Examples" );
    	accuracyPlot.setAxisLabelY( "Classification Accuracy" );
    	
    	accuracyPlot.getCrosshairPainter().setVisible( false );
    	
    	accuracyPainter = new TrackPainter( );
    	accuracyPainter.setShowPoints(0, false);
    	accuracyPainter.setLineColor(0, GlimpseColor.getRed());
    	
    	accuracyPlot.addPainter( accuracyPainter );
    	
        // create an Axis2D to define the bounds of the plotting area
        Axis2D axis = new Axis2D( );

        double minX = -0.8;
        double maxX = 0.8;
        double minY = -8.0;
        double maxY = 0.0;

        // set the axis bounds (display all the data initially)
        axis.set( minX, maxX, minY, maxY );

        // create a layout to draw the geographic data
        GlimpseAxisLayout2D treeVisualization = new GlimpseAxisLayout2D( "TreeVisualization", axis );

        // add an axis mouse listener to the layout so that it responds to user pans/zooms
        treeVisualization.addGlimpseMouseAllListener( new AxisMouseListener2D( )
        {
            // double horizontal zoom factor
            public void zoom( Axis1D axis, boolean horizontal, int zoomIncrements, int posX, int posY )
            {
                if ( horizontal ) super.zoom( axis, horizontal, zoomIncrements*2, posX, posY );
                else super.zoom( axis, horizontal, zoomIncrements, posX, posY );
            }
        } );

        treeVisualization.addPainter( new BorderPainter( ) );
        
        // add a solid color background painter to the plot
        treeVisualization.addPainter( new BackgroundPainter( ).setColor( GlimpseColor.getWhite( ) ) );

        // add a painter to display the tree structure
        treePainter = new VisualizableNodePainter( );
        treeVisualization.addPainter( treePainter );
        
        parent.addLayout( treeVisualization );
        parent.addLayout( accuracyPlot );
        
        treeVisualization.setLayoutData( "cell 0 1, push, grow" );
        accuracyPlot.setLayoutData( "cell 0 0, pushx, growx, height 200!" );

        return parent;
    }
    
    public void addAccuracy( int n, double accuracy )
    {
    	accuracyPainter.addPoint( 0, n, n, accuracy, n );
    	
    	double maxX = accuracyPlot.getAxisX().getMax();
    	double absMaxX = accuracyPlot.getAxisX().getAbsoluteMax();
    	
    	if ( maxX == absMaxX )
    	{
    		accuracyPlot.setMaxX( n );
    		accuracyPlot.setAbsoluteMaxX( n );
    		accuracyPlot.validate();
    	}
    }

    public void setNode( VisualizableNode node )
    {
        this.treePainter.setVisualizableNode( node );
    }
}