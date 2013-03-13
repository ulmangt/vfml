#ifndef BELIEFNETH
#define BELIEFNETH

/** \ingroup CoreAPI BeliefNet
*/


#include "ExampleSpec.h"
#include "Example.h"
#include "../util/lists.h"
/*#include <stdio.h>*/

/** \file 

\brief A Belief Net Structure with CPT local models

This is the interface for creating, using, printing, & serializing
Belief Nets (aka Bayesian Networks). This document will first describe
the properties of BeliefNets and then of BeliefNetNodes. A belief net
is a compact representation of a joint probability distribution of all
of the variables in a domain. For each variable there is a local model
(represented by a BeliefNetNode) that models the probability of the
various values of that varable given the values of the variables that
affect them. A BeliefNet is an acyclic graph of the nodes, where an
edge represets (loosely) that the variable at the source affects the
varable at the target. The exact semantics of an edge are a bit more
complex. For a more detailed discussion see <a
href="http://citeseer.nj.nec.com/heckerman96tutorial.html">David
Heckerman's Tutorial</a>

\wish A version of this that uses DecisionTree local models instead of
full CPTs.  This would also need a new structure learning tool (a
modification of \ref bnlearn).

*/


/** \name BeliefNetNode
*/
/**@{*/


/**\brief Belief net node with full CPTs for local models.

See BeliefNet.h for more detail.
*/
typedef struct _BeliefNetNode_ {
   int numAllocatedRows;
   int numParentCombinations;

   // indexed first by parents in order in counting order
   //  second index on value for this variable
   float **eventCounts;

   // the number of times this parent combination occured.
   float *rowCounts;

   // This is the sum of all the rowCounts;
   float numSamples;

   ExampleSpecPtr spec;
   int attributeID;

   struct _BeliefNet_ *bn;

   VoidListPtr parentIDs;
   VoidListPtr childrenIDs;
   void *userData;

   /* used internally by graph algorithms or inference algorithms,
       do not use it for things you want preserved across calls,
       use userData for that */
   void *tmpInternalData;
} BeliefNetNodeStruct, *BeliefNetNode;


//BeliefNetNode BNNodeNew(void);

void BNNodeFree(BeliefNetNode bnn);

// NOTE! Both nodes better have CPTs allocated and they better be the same
void BNNodeSetCPTFrom(BeliefNetNode target, BeliefNetNode source);

// Note: this does NOT clone user data, merely copies the pointer
BeliefNetNode BNNodeClone(BeliefNetNode bnn);

/** \brief Adds the specified node as a parent to bnn. 

Both nodes should be from the same BeliefNet structure.
*/
void BNNodeAddParent(BeliefNetNode bnn, BeliefNetNode parent);
//void BNNodeAddChild(BeliefNetNode bnn, BeliefNetNode child);

/** \brief Returns the index of parent in bnn's parent list.

Returns -1 if parent is not one of node's parents. 
*/
int BNNodeLookupParentIndex(BeliefNetNode bnn, BeliefNetNode parent);

/** \brief Looks through the parent list of bnn for a node with node id of 'id'.

See BNNodeGetID for more info. 
*/
int BNNodeLookupParentIndexByID(BeliefNetNode bnn, int id);

/** \brief Removes the node with index 'parentIndex' from bnn's parent list.

To remove the node 'parent' call BNNodeRemoveParent(bnn, BNNodeLookupParentIndex(bnn, parent)).
*/
void BNNodeRemoveParent(BeliefNetNode bnn, int parentIndex);

/** \brief Returns the parent at position 'index' in the node's parent list. */
BeliefNetNode BNNodeGetParent(BeliefNetNode bnn, int parentIndex);

/** \brief Returns the node id of the node at position 'index' in the node's parent list. */
int BNNodeGetParentID(BeliefNetNode bnn, int parentIndex);

/** \brief Returns the number of nodes in the target node's parent list. */
int BNNodeGetNumParents(BeliefNetNode bnn);

/** \brief Returns the number of nodes in the target node's child list. */
int BNNodeGetNumChildren(BeliefNetNode bnn);

/** \brief Returns 1 if and only if parent is in the node's parent list. */
int BNNodeHasParent(BeliefNetNode bnn, BeliefNetNode parent);

/** \brief Returns 1 if and only if one of the node's parents has the specified node id. */
int BNNodeHasParentID(BeliefNetNode bnn, int parentID);

void BNNodeAddValue(BeliefNetNode bnn, char *valueName);
int BNNodeLookupValue(BeliefNetNode bnn, char *valueName);

/** \brief Returns the number of values that the variable represented by the node can take. */
int BNNodeGetNumValues(BeliefNetNode bnn);

/** \brief Returns the number of parameters in the node's CPT. */
int BNNodeGetNumParameters(BeliefNetNode bnn);

/** \brief Returns the name of the node. */
char *BNNodeGetName(BeliefNetNode bnn);

/** \brief Returns 1 if and only if the two nodes have the same parents in the same order.

\bug Only returns 1 if the parents are in the same order, but the order probably shouldn't matter.
*/
int BNNodeStructureEqual(BeliefNetNode bnn, BeliefNetNode otherNode);


/** \brief Allocates memory for bnn's CPT and zeros the values.

This allocates enough memory to hold one float for each value of the
variable associated with the node for each parent combination (an
amount of memory that is exponential in the number of parents).  This
should be called once all parents are in place.  */
void BNNodeInitCPT(BeliefNetNode bnn);


/** \brief Sets the value of all CPT entries to zero. 

Can be called after InitCPT to reset all the table's values to 0.
*/
void BNNodeZeroCPT(BeliefNetNode bnn);

/** \brief Frees any memory being used by the node's CPTs.

This should be called before changing the node's parents. After a call
to this function, you should call BNNodeInitCPT for the node before
making any calls that might try to access the CPT (adding samples,
doing inference, smoothing probability, comparin networks, etc).

*/
void BNNodeFreeCPT(BeliefNetNode bnn);

// returns the attribute id of the node (once the node's identity is set)
int BNNodeGetID(BeliefNetNode bnn);

void BNNodeSetUserData(BeliefNetNode bnn, void *data);
//inline void *BNNodeGetUserData(BeliefNetNode bnn) {return bnn->userData;}
#define BNNodeGetUserData(bnn) (((BeliefNetNode)bnn)->userData)


/** \brief Increments the count of the appropriate CPT element by 1. 

Looks in example to get the values for the parents and the value for
the variable.  If any of these are unknown changes nothing, prints a
low priority warning message, and returns -1 where applicable.

*/
void  BNNodeAddSample(BeliefNetNode bnn, ExamplePtr e);

/** \brief Calls BNNodeAddSample for each example in the list. */
void  BNNodeAddSamples(BeliefNetNode bnn, VoidListPtr samples);

/** \brief Increments the count of the appropriate CPT element by weight.

Looks in example to get the values for the parents and the value for
the variable.  If any of these are unknown changes nothing, prints a
low priority warning message, and returns -1 where applicable.
*/
void  BNNodeAddFractionalSample(BeliefNetNode bnn, ExamplePtr e, float weight);

/** \brief Calls BNNodeAddFractionalSample for each example in the list. */
void  BNNodeAddFractionalSamples(BeliefNetNode bnn, VoidListPtr samples, float weight);

/** \brief Returns the number of samples that have been added to the node with the same parent combination as in e. 

Looks in example to get the values for the parents and the value for
the variable.  If any of these are unknown changes nothing, prints a
low priority warning message, and returns -1 where applicable.
*/
float BNNodeGetCPTRowCount(BeliefNetNode bnn, ExamplePtr e);


/** \brief Returns the marginal probability of the appropriate value of the variable.

That is, the sum over all rows of the number of counts for that value
divided by the sum over all rows of the number of counts in the row.

Looks in example to get the values for the parents and the value for
the variable.  If any of these are unknown changes nothing, prints a
low priority warning message, and returns -1 where applicable.

*/
float BNNodeGetP(BeliefNetNode bnn, int value);
void  BNNodeSetCPIndexed(BeliefNetNode bnn, int row, int value,
                                                      float probability);
float BNNodeGetCPIndexed(BeliefNetNode bnn, int row, int value);

/** \brief Get the probability of the value of the target variable given the values of the parent variables. 

Looks in example to get the values for the parents and the value for
the variable.  If any of these are unknown changes nothing, prints a
low priority warning message, and returns -1 where applicable.
*/
float BNNodeGetCP(BeliefNetNode bnn, ExamplePtr e);

/** \brief Sets the probability without affecting the sum of the CPT row for the parent combination.

This means that the probability has the same prior weight before and
after a call to this. Put another way, the set probability is
equivilant to having seen the same number of samples at the new
probability as at the old.

Looks in example to get the values for the parents and the value for
the variable.  If any of these are unknown changes nothing, prints a
low priority warning message, and returns -1 where applicable.
*/

void  BNNodeSetCP(BeliefNetNode bnn, ExamplePtr e, float probability);

// Reset all row counts to 'strength'.  This effectively sets the confidence
//  in the current probabilities to be equal to what you would have after
//  seeing 'strength' samples which add up to the current probabilities.
//  (which may not be possible, but that's ok!)
void BNNodeSetPriorStrength(BeliefNetNode bnn, double strength);
void BNNodeSmoothProbabilities(BeliefNetNode bnn, double strength);

void BNNodeSetRandomValueGivenParents(BeliefNetNode bnn, ExamplePtr e);


/** \brief Returns the number of samples that have been added to the belief net node. */
float BNNodeGetNumSamples(BeliefNetNode bnn);

/** \brief Returns the number rows in the node's CPT. This is the number of parent combinations. */
int BNNodeGetNumCPTRows(BeliefNetNode bnn);

/**@}*/


/** \name BeliefNet
*/
/**@{*/

/** \brief Belief network ADT.

See BeliefNet.h for more detail.
 */
typedef struct _BeliefNet_ {
   char *name;
   ExampleSpecPtr spec;
   VoidListPtr nodes;

   /* these next two contain cached values, will be 0 and -1 till filled */
   VoidListPtr topoSort;
   int hasCycle;

   void *userData;
} BeliefNetStruct, *BeliefNet;

/** \brief Creates a new belief net with no nodes. */
BeliefNet BNNew(void);

/** \brief Frees the memory associated with the belief net and all nodes. */
void BNFree(BeliefNet bn);

/** \brief Makes a copy of the belief net and all nodes.*/
BeliefNet BNClone(BeliefNet bn);

/** \brief Makes a copy of the belief net and all nodes, but does not copy the local models at the nodes. */
BeliefNet BNCloneNoCPTs(BeliefNet bn);

/** \brief Makes a new belief net from the example spec.

All attributes in the spec should be discrete. This adds a node, with
the appropriate values, to the net for each variable in the spec.  The
resulting network has no edges and zeroed CPTs


*/
BeliefNet BNNewFromSpec(ExampleSpecPtr es);

int BNStructureEqual(BeliefNet bn, BeliefNet otherNet);

/** \brief Returns the symetric difference in the structures.

This is defined as the sum for i in nodes of the number of parents
that node i of bn has but node i of otherBN does not have plus the
number of parents that node i of otherBN has that node i of bn does
not have.
*/
int BNGetSimStructureDifference(BeliefNet bn, BeliefNet otherNet);

/** \brief Set's the Belief net's name.

This doesn't really affect anything (except it is recorded if you
write out the belief net), but using it may make you feel better.
*/
void BNSetName(BeliefNet bn, char *name);

// Needed for reading and writing examples that will work with the net

/** \brief Returns the ExampleSepc that is associated with the belief net.

The spec will be automatically created when you read the network from
disk or as you add nodes to the net and values to the nodes.
*/
ExampleSpec *BNGetExampleSpec(BeliefNet bn);

//void BNSetUserData(BeliefNet bn, int nodeID, void *data);
//void *BNGetUserData(BeliefNet bn, int nodeID);

//void BNAddNode(BeliefNet bn, BeliefNetNode bnn, char *nodeName);

BeliefNetNode BNNewNode(BeliefNet bn, char *nodeName);

/* Looks up the node by name.

This is an O(N) operation. If it isn't found returns NULL. */
BeliefNetNode BNLookupNode(BeliefNet bn, char *name);

/** \brief Gets the node with the associated index.

This is 0 based (like a C array). */
BeliefNetNode BNGetNodeByID(BeliefNet bn, int id);

/** \brief Returns the number of nodes in the Belief Net. */
int BNGetNumNodes(BeliefNet bn);

/** \brief Returns nodes by their order in a topological sort.

If the nodes can not be topologically sorted (perhaps because there is
a cycle) this function returns 0.

Note that this function caches (and uses a cache) of the topological
sort and that you might want to call BNFlushStructureCache before
calling this if you've changed the structure of the net since the
cache was filled.

*/
BeliefNetNode BNGetNodeByElimOrder(BeliefNet bn, int index);

/** \brief Returns 1 if and only if there is a cycle in the graphical structure of the belief net.

Note that this function caches (and uses a cache) of the topological
sort and that you might want to call BNFlushStructureCache before
calling this if you've changed the structure of the net since the
cache was filled.
*/
int BNHasCycle(BeliefNet bn);

/** \brief Needs to be called anytime you change network structure.

That is, any time you add nodes, add or remove edges after calling
BNHasCycle, or any ElimOrder stuff.  The reason is that these two
classes of functions cache topological sorts of the network and
changing the structure invalidates these caches.  In prinicipal, all
of the functions that modify structure could be modified to
automatically call this.  I think things were done this way for
efficiency (but I am not sure it actually helps efficiency...

*/ 
void BNFlushStructureCache(BeliefNet bn);

/* \brief Calls BNNodeZeroCPT on every node in the network. */
void BNZeroCPTs(BeliefNet bn);

//void BNPrint(BeliefNet bn, FILE *out);
//void BNPrintStats(BeliefNet bn, FILE *out);

/** \brief Modifies all the CPTs in the network by adding a count to the approprite parameters.

See the BeliefNetNode functions for a more detailed description
of how the CPTs are represented and handled.
*/
void BNAddSample(BeliefNet bn, ExamplePtr e);

/** \brief Calls BNAddSample for every example in the list */
void BNAddSamples(BeliefNet bn, VoidListPtr samples);

/** \brief Modifies all the CPTs in the network by adding a weighted count to the approprite parameters.

See the BeliefNetNode functions for a more detailed description
of how the CPTs are represented and handled.
*/
void BNAddFractionalSample(BeliefNet bn, ExamplePtr e, float weight);

/** \brief Calls BNAddFractionalSample for every example in the list */
void BNAddFractionalSamples(BeliefNet bn, VoidListPtr samples, float weight);

/* \brief Returns the loglikelihood of the example given the network and parameters. */
float BNGetLogLikelihood(BeliefNet bn, ExamplePtr e);

/** \brief Returns the sum over all nodes of the number of independent parameters in the CPTs.

This is different from the total number of parameters because one of
the parameters in each row can be determined from the values of the
others, and so is not independent.
*/
long BNGetNumIndependentParameters(BeliefNet bn);

/** \brief Returns the sum over all nodes of the number of parameters in the CPTs. */
long BNGetNumParameters(BeliefNet bn);

/** \brief Returns the number of parameters in the node with the most parameters. */
long BNGetMaxNodeParameters(BeliefNet bn);


/** \brief Samples from the distribution represented by bn.

You are responsible for freeing the returned example (using
ExampleFree when you are done with it.
*/
ExamplePtr BNGenerateSample(BeliefNet bn);

/** \brief Sets prior parameter strength as if some examples have been
seen.

Multiplies all the counts in all of the network's CPTs so that each
parent combination has the equivilant of strength samples, divided
acording to that combination's distribution in the network.

Hrm, does that confuse you too?
*/
void BNSetPriorStrength(BeliefNet bn, double strength);

/** \brief Adds a number of samples equal to strength to each CPT
entry in the network.

This effectivly smooths the probabilities towards uniform.
*/
void BNSmoothProbabilities(BeliefNet bn, double strength);

/** \brief Allows you to store an arbitrary pointer on the BeliefNet.

You are responsible for managing any memory that it points to.
*/
void BNSetUserData(BeliefNet bn, void *data);

// void *BNGetUserData(BeliefNet bn) {return bn->userData;}
/** \brief Allows you to store an arbitrary pointer on the BeliefNet.

You are responsible for managing any memory that it points to.
*/
#define BNGetUserData(bn) (((BeliefNet)bn)->userData)

/** \brief  Reads a Belief net from the named file.  

The file should contain a net in <a
href="../appendixes/bif.htm">Bayesian Interchange Format</a> (BIF).
*/
BeliefNet BNReadBIF(char *fileName);


/** \brief  Reads a Belief net from a file pointer.

The file pointer should be opened for reading and should contain a net
in <a href="../appendixes/bif.htm">Bayesian Interchange Format</a>
(BIF).  */
BeliefNet BNReadBIFFILEP(FILE *file);

/** \brief Writes the belief net to the file.

Out should be a file open for writing, pass stdout to write to the
console.  The net it written in <a
href="../appendixes/bif.htm">Bayesian Interchange Format</a> (BIF).

*/
void BNWriteBIF(BeliefNet bn, FILE *out);

/** \brief Prints some information about the net to stdout.

The information includes num nodes, min max avg num parents, etc.
*/
void BNPrintStats(BeliefNet bn);




/**************************************************
  Mattr added below here and in .c.
  Geoff, if you approve, I guess move up where they belong
**************************************************/
// Allocates and fills in eventCounts and rowCounts
void BNNodeCopyCPT(BeliefNetNode bnn, float ***eventCounts, float **rowCounts);
// Restores from eventCounts and rowCounts back into the node
void BNNodeRestoreCPT(BeliefNetNode bnn, float **eventCounts, float *rowCounts);
// Just allocates
void BNNodeAllocateCPT(BeliefNetNode bnn, float ***eventCounts, float **rowCounts);
// Frees the result of one of the above two functions
void BNNodeFreeCopiedCPT(BeliefNetNode bnn, float **eventCounts, float *rowCounts);

void BNNodeRemoveParentByID(BeliefNetNode bnn, int parentID);
void BNNodeRemoveEdge(BeliefNetNode parent, BeliefNetNode child);

// If doDirichlet, then the event and rowCounts will be dirichlet parameters
//  otherwise, they will be shrunken event and rowCounts
void  BNNodeShrink(BeliefNetNode bnn, VoidListPtr data, int doDirichlet);
void BNNodeTruncateToNParents(BeliefNetNode bnn, VoidListPtr data, 
                               int maxParents);

void BNRemoveNode(BeliefNet bn, BeliefNetNode node);

void BNNodeCPTGetP(BeliefNetNode bnn, ExamplePtr e, double *p);
double BNNodeCPTGetCP(BeliefNetNode bnn, ExamplePtr e);
void BNNodeGetJointProb(BeliefNetNode parent, BeliefNetNode child, double p[2][2]);

/**@}*/


/** \name Inference with Likelihood Sampling

The idea of likelihood sampling is that you have a set of query variables (specified by an example with some of the values unknown) and the system randomly generates samples for the query variables given the values of the non-query variables. After 'enough' samples the distribution of the samples generated for the query variables will be a good match to the 'true' distribution according to the net. The number of samples required can be very large, especially when dealing with events that don't occur often.
*/
/**@{*/

/** \brief Set up likelihood sampling and return place holder network.

Returns a new belief net with CPT set to start to acumulate samples
for the unknown variables in e, you should free this net when you are
done with it. Once the net is created you should add as many samples
to it as you like using BNAddLikelihoodSamples and then check the CPTs
at the appropriate nodes for the generated distributions.
*/
BeliefNet BNInitLikelihoodSampling(BeliefNet bn, ExamplePtr e);

/** \brief Adds the requested number of samples to newNet.

newNet should have been created by a call to
BNInitiLikelihoodSampling.  Adds the requested number of samples for
the unknown variables in e using the distributions in bn.
*/
void BNAddLikelihoodSamples(BeliefNet bn, BeliefNet newNet, ExamplePtr e, int numSamples);

/** \brief Combines a call to BNInitiLikelihoodSampling with a call to BNAddLikelihoodSamples. */
BeliefNet BNLikelihoodSampleNTimes(BeliefNet bn, ExamplePtr e, int numSamples);
/**@}*/


#endif /* BELIEFNETH */
