package edu.gmu.vfml.test;

import weka.classifiers.trees.CVFDT;
import weka.classifiers.trees.VFDT;
import edu.gmu.vfml.data.RandomDataGenerator;

public class CVFDTVisualization extends VFDTVisualization
{

	public static void main(String[] args) throws Exception
	{
		new CVFDTVisualization().run();
	}

	@Override
	protected VFDT buildClassifier(RandomDataGenerator generator) throws Exception
	{
		// build a CVFDT classifier
		final CVFDT classifier = new CVFDT();
		classifier.setConfidenceLevel(1e-6);
		classifier.setNMin(30);
		classifier.setSplitRecheckInterval(5000);
		classifier.initialize(generator.getDataset());

		return classifier;
	}
}
