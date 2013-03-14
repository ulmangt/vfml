package edu.gmu.vfml.classifier;

import java.util.Enumeration;

import weka.classifiers.Classifier;
import weka.core.Capabilities;
import weka.core.Capabilities.Capability;
import weka.core.Instance;
import weka.core.Instances;
import weka.core.TechnicalInformation;
import weka.core.TechnicalInformation.Field;
import weka.core.TechnicalInformation.Type;
import weka.core.TechnicalInformationHandler;

/**
 * 
 * @see weka.classifiers.trees.Id3
 * @author ulman
 */
public class VFML extends Classifier implements TechnicalInformationHandler
{
    private static final long serialVersionUID = 1L;

    /**
     * Returns a string describing the classifier.
     * @return a description suitable for the GUI.
     */
    public String globalInfo( )
    {
        //@formatter:off
        return "Class for constructing an unpruned decision tree based on the VFDT " +
               "algorithm. For more information see: \n\n" +
               getTechnicalInformation( ).toString( );
        //@formatter:on
    }

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

        info.setValue( Field.AUTHOR, "Domingos, Pedro" );
        info.setValue( Field.YEAR, "2000" );
        info.setValue( Field.TITLE, "Mining high-speed data streams" );
        info.setValue( Field.JOURNAL, "Proceedings of the sixth ACM SIGKDD international conference on Knowledge discovery and data mining" );
        info.setValue( Field.SERIES, "KDD '00" );
        info.setValue( Field.ISBN, "1-58113-233-6" );
        info.setValue( Field.LOCATION, "Boston, Massachusetts, USA" );
        info.setValue( Field.PAGES, "71-80" );
        info.setValue( Field.URL, "http://doi.acm.org/10.1145/347090.347107" );
        info.setValue( Field.PUBLISHER, "ACM" );

        return info;
    }

    /**
     * Returns default capabilities of the classifier.
     *
     * @return the capabilities of this classifier
     */
    public Capabilities getCapabilities( )
    {
        Capabilities result = super.getCapabilities( );
        result.disableAll( );

        // attributes
        result.enable( Capability.NOMINAL_ATTRIBUTES );

        // class
        result.enable( Capability.NOMINAL_CLASS );
        result.enable( Capability.MISSING_CLASS_VALUES );

        // instances
        result.setMinimumNumberInstances( 0 );

        return result;
    }

    /**
     * Builds Id3 decision tree classifier.
     *
     * @param data the training data
     * @exception Exception if classifier can't be built successfully
     */
    @Override
    public void buildClassifier( Instances data ) throws Exception
    {

        // can classifier handle the data?
        getCapabilities( ).testWithFail( data );

        // remove instances with missing class
        data = new Instances( data );
        data.deleteWithMissingClass( );

        makeTree( data );
    }

    /**
     * Method for building an Id3 tree.
     *
     * @param data the training data
     * @exception Exception if decision tree can't be built successfully
     */
    private void makeTree( Instances data ) throws Exception
    {
        makeTree( data.enumerateInstances( ) );
    }
    
    private void makeTree( Enumeration data )
    {
        while( data.hasMoreElements( ) )
        {
            Instance instance = (Instance) data.nextElement( );
        }
    }
}
