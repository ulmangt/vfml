package weka.classifiers.trees;

import static com.metsci.glimpse.util.logging.LoggerUtils.logWarning;

import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;
import java.util.Vector;
import java.util.logging.Logger;

import weka.core.Attribute;
import weka.core.Instance;
import weka.core.Instances;
import weka.core.Option;
import weka.core.TechnicalInformation;
import weka.core.Utils;
import weka.core.TechnicalInformation.Field;
import weka.core.TechnicalInformation.Type;
import edu.gmu.vfml.tree.CNode;
import edu.gmu.vfml.tree.InstanceId;
import edu.gmu.vfml.tree.Node;

/**
 * <!-- globalinfo-start -->
 * will be automatically replaced
 * <!-- globalinfo-end -->
 * 
 *  <!-- technical-bibtex-start -->
 * will be automatically replaced
 * <!-- technical-bibtex-end -->
 * 
 *  <!-- options-start -->
 * will be automatically replaced
 * <!-- options-end -->
 * 
 * @see weka.classifiers.trees.VFDT
 * @author ulman
 */
public class CVFDT extends VFDT
{
    private static final Logger logger = Logger.getLogger( CVFDT.class.getName( ) );

    private static final long serialVersionUID = 1L;

    /**
     * Linked list of instances currently inside the CVFDT learning window.
     */
    protected LinkedList<InstanceId> window;
    
    /**
     * The maximum size of the window list.
     */
    protected int windowSize;
    
    /**
     * The number of data instances between rechecks of the validity of all
     * alternative trees. This is a global count on instances (see
     * splitValidityCounter).
     */
    protected int splitRecheckInterval = 20000;
    
    /**
     * Every altTestModeInterval instances, CNodes enter a test state where they
     * use the next altTestModeDuration instances to evaluate whether or not
     * to discard the current tree in favor of one of the Node's alternative trees.
     */
    protected int altTestModeInterval = 9000;
    protected int altTestModeDuration = 1000;
    
    transient protected int largestNodeId;
    transient protected int splitValidityCounter;

    /**
     * Lists the command line options available to this classifier.
     */
    @Override
    @SuppressWarnings( { "rawtypes", "unchecked" } )
    public Vector listOptionsVector( )
    {
        Vector list = super.listOptionsVector( );
        list.addElement( new Option( "\tWindow Size.\n", "W", 1, "-W <window size>" ) );
        list.addElement( new Option( "\tSplit Recheck Interval.\n", "F", 1, "-F <split recheck interval>" ) );
        list.addElement( new Option( "\tTest Mode Interval.\n", "I", 1, "-I <test mode interval>" ) );
        list.addElement( new Option( "\tTest Mode Duration.\n", "D", 1, "-D <test mode duration>" ) );
        return list;
    }

    /**
     * Parses a given list of options.
     * 
     * @param options the list of options as an array of strings
     * @throws Exception if an option is not supported
     */
    @Override
    public void setOptions( String[] options ) throws Exception
    {
        super.setOptions( options );
        
        String windowSizeString = Utils.getOption( 'W', options );
        if ( !windowSizeString.isEmpty( ) )
        {
            windowSize = Integer.parseInt( windowSizeString );
        }
        
        String splitRecheckString = Utils.getOption( 'F', options );
        if ( !splitRecheckString.isEmpty( ) )
        {
            splitRecheckInterval = Integer.parseInt( splitRecheckString );
        }
        
        String altTestModeIntervalString = Utils.getOption( 'I', options );
        if ( !splitRecheckString.isEmpty( ) )
        {
            altTestModeInterval = Integer.parseInt( altTestModeIntervalString );
        }
        
        String altTestModeDurationString = Utils.getOption( 'D', options );
        if ( !splitRecheckString.isEmpty( ) )
        {
            altTestModeDuration = Integer.parseInt( altTestModeDurationString );
        }
    }
    
    /**
     * Gets the current settings of the Classifier.
     *
     * @return an array of strings suitable for passing to setOptions
     */
    @Override
    public List<String> getOptionsList( )
    {
        List<String> options = super.getOptionsList( );
        
        options.add( "-W" );
        options.add( String.valueOf( windowSize ) );
        
        options.add( "-F" );
        options.add( String.valueOf( splitRecheckInterval ) );
        
        options.add( "-I" );
        options.add( String.valueOf( altTestModeInterval ) );
        
        options.add( "-D" );
        options.add( String.valueOf( altTestModeDuration ) );
        
        return options;
    }
    
    /**
     * Returns an instance of a TechnicalInformation object, containing 
     * detailed information about the technical background of this class,
     * e.g., paper reference or book this class is based on.
     * 
     * @return the technical information about this class
     */
    @Override
    public TechnicalInformation getTechnicalInformation( )
    {
        TechnicalInformation info = new TechnicalInformation( Type.ARTICLE );

        info.setValue( Field.AUTHOR, "Pedro Domingos, Laurie Spencer, and Geoff Hulten" );
        info.setValue( Field.YEAR, "2001" );
        info.setValue( Field.TITLE, "Mining Time-Changing Data Streams" );
        info.setValue( Field.JOURNAL, "Proceedings of the seventh ACM SIGKDD international conference on Knowledge discovery and data mining" );
        info.setValue( Field.SERIES, "KDD '01" );
        info.setValue( Field.ISBN, "1-58113-391-X" );
        info.setValue( Field.LOCATION, "Boston, Massachusetts, USA" );
        info.setValue( Field.PAGES, "97-106" );
        info.setValue( Field.URL, "http://dl.acm.org/citation.cfm?id=502529" );
        info.setValue( Field.PUBLISHER, "ACM" );

        return info;
    }

    @Override
    public CNode getRoot( )
    {
        return (CNode) root;
    }
    
    @Override
    public Node newNode( Instances instances )
    {
        return new CNode( instances, classAttribute, ++largestNodeId );
    }

    /**
     * Method for building an CVFDT tree.
     *
     * @param data the training data
     * @exception Exception if decision tree can't be built successfully
     */
    @Override
    protected void makeTree( Instances data ) throws Exception
    {
        makeTree( data.enumerateInstances( ) );
    }
    
    /**
     * Perform classifier initialization steps.
     */
    protected void initialize( Instances data ) throws Exception
    {
        super.initialize( data );
        
        this.window = new LinkedList<InstanceId>( );
        this.largestNodeId = 0;
        this.splitValidityCounter = 0;
    }

    @SuppressWarnings( "rawtypes" )
    protected void makeTree( Enumeration data )
    {
        while ( data.hasMoreElements( ) )
        {
            try
            {
                // retrieve the next data instance
                Instance instance = ( Instance ) data.nextElement( );

                // traverse the classification tree to find the leaf node for this instance
                Node node = getLeafNode( instance );

                //TODO while incrementing node counts, keep track of when the
                //     node should go in/out of testing mode
                
                // update the counts associated with this instance
                // unlike VFDT, we start at the root because we will reach multiple
                // leaf nodes in the various alternative trees
                getRoot( ).traverseAndIncrementCounts( instance );
                
                // add the new instance to the window and remove old instance (if necessary)
                updateWindow( instance );

                // check whether or not to split the node on an attribute
                if ( node.getCount( ) % nMin == 0 )
                {
                    checkNodeSplit( instance, node );
                }
                
                if ( ++splitValidityCounter % splitRecheckInterval == 0 )
                {
                    //TODO fill in alternative subtree start logic    
                }
            }
            catch ( Exception e )
            {
                logWarning( logger, "Trouble processing instance.", e );
            }
        }
    }
    
    protected void updateWindow( Instance instance )
    {
        // add the new instance to the window
        // tag it with the id of the largest currently existing node
        window.addLast( new InstanceId( instance, largestNodeId ) );
        
        // drop the oldest instance from the window
        if ( window.size( ) > windowSize )
        {
            InstanceId oldInstanceId = window.removeFirst( );
            int oldId = oldInstanceId.getId( );
            
            // iterate through the tree (and all alternative trees) and decrement
            // counts if the node's id is less than or equal to oldId
            getRoot( ).traverseAndDecrementCounts( instance, oldId );
        }
    }
    
    @Override
    protected void splitNode( Node node, Attribute attribute, Instance instance )
    {
        ((CNode) node).split( attribute, instance, ++largestNodeId );
    }
}
