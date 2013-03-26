package weka.classifiers.trees;

import java.util.Enumeration;

import weka.core.Instances;
import weka.core.TechnicalInformation;
import weka.core.TechnicalInformation.Field;
import weka.core.TechnicalInformation.Type;
import edu.gmu.vfml.tree.CNode;
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
    private static final long serialVersionUID = 1L;
    
    /**
     * Returns an instance of a TechnicalInformation object, containing 
     * detailed information about the technical background of this class,
     * e.g., paper reference or book this class is based on.
     * 
     * @return the technical information about this class
     */
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
    public Node newNode( Instances instances )
    {
        return new CNode( instances, classAttribute );
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

    @Override
    @SuppressWarnings( "rawtypes" )
    protected void makeTree( Enumeration data )
    {
        
    }
}
