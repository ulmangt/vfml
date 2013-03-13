#ifndef CVFDT_H
#define CVFDT_H

typedef struct _CVFDT_ 
{
  VFDTPtr vfdt;
  DecisionTreePtr dtreeNode;
  int examplesSeen;
  VoidAListPtr                       
  children; // child cvfdt nodes
  long uid; // unique identifier, incremented sequentially
  int hasChanged;
  unsigned long nodeId; // ex 121 means choose child 1 at level 1, 2 at level 2...
  VoidAListPtr altSubtreeInfos;
} CVFDT, *CVFDTPtr;

typedef struct _CVFDT_Alt_Info
{
  CVFDTPtr subtree;
  int status; // 0 is growing, 1 is testing
  int exampleCount;
  int numErrors;
  int numTested;
  float closestErr;

} CVFDTAltInfo, *CVFDTAltInfoPtr;

typedef struct _CVFDT_Example_
{
  ExamplePtr example;
  unsigned int oldestNode;
  int inQNode;
} CVFDTExample, *CVFDTExamplePtr;

CVFDTPtr CVFDTCreateRoot( ExampleSpecPtr spec, float splitConfidence, float tieConfidence);
CVFDTPtr CVFDTNew( VFDTPtr vfdt, DecisionTreePtr dt, long uid );
void CVFDTAddChild( CVFDTPtr cvfdt, DecisionTreePtr dtree );
void CVFDTProcessExamples(CVFDTPtr cvfdt, FILE *input);
void CVFDTPrint( CVFDTPtr cvfdt, FILE *out );
CVFDTPtr CVFDTOneStepClassify(CVFDTPtr cvfdt, ExamplePtr e);
DecisionTreePtr CVFDTGetLearnedTree(CVFDTPtr cvfdt);
void CVFDTForgetExample(CVFDTPtr cvfdt, CVFDTExamplePtr ce);
#endif



















