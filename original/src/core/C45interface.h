#ifndef C45INTERFACE
#define C45INTERFACE

#include "DecisionTree.h"
#include "Example.h"


/** \ingroup CoreAPI DecisionTree
*/

/** \file

\brief Calls the C4.5 decision tree learning system and returns the learned tree.

An interface that writes the examples to a file, calls C4.5 on them,
and returns the tree that C4.5 learns. See the <a
href="examples/interface_with_c45.htm">example program</a> for more
information on how to use this interface. C4.5 must be in your path
for this to work, you can grab it from Professor Quinlan's homepage at
http://www.cse.unsw.edu.au/~quinlan/.

*/

/** \brief Makes the call and returns the tree */
DecisionTreePtr C45Learn(ExampleSpecPtr es, VoidListPtr examples);

#endif
