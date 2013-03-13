#include <stdio.h>
#include <math.h>
#include "ExampleGroupStats.h"
#include "memory.h"

#define lg(x)       (log(x) / log(2))
#define kOneOverE (0.367879441)

static float *_MakeClassCounts(ExampleSpecPtr es) {
   float *tmp;
   int i;

   tmp = MNewPtr(sizeof(long) * ExampleSpecGetNumClasses(es));

   for(i = 0 ; i < ExampleSpecGetNumClasses(es) ; i++) {
      tmp[i] = 0;
   }

   return tmp;
}

BinPtr BinNew(ExampleSpecPtr spec) {
   BinPtr bin;

   bin = MNewPtr(sizeof(Bin));
   bin->classTotals = _MakeClassCounts(spec);
   bin->upperBound = 0;
   bin->lowerBound = 0;
   bin->exampleCount = 0;
   bin->boundryClass = -1;
   bin->boundryCount = 0;

   return bin;
}

void BinFree(BinPtr bin) {
   MFreePtr(bin->classTotals);
   MFreePtr(bin);
}

void BinWrite(BinPtr bin, FILE *out) {
   fprintf(out, "Bin - Low %.3f Up %.3f B-Class %d B-Count %ld E-Count %.2f\n", bin->lowerBound, bin->upperBound, bin->boundryClass, bin->boundryCount, bin->exampleCount);
}

ContinuousTrackerPtr ContinuousTrackerNew(ExampleSpecPtr es) {
   ContinuousTrackerPtr ct = MNewPtr(sizeof(ContinuousTracker));

   ct->spec = es;
   ct->classTotals = _MakeClassCounts(es);
   ct->exampleCount = 0;
   ct->bins = VLNew();

   ct->min = ct->max = ct->initMinMax = 0;
   ct->timesPrunedCount = 0;
   ct->addingNewBins = 1;
   
   return ct;
}

void ContinuousTrackerFree(ContinuousTrackerPtr ct) {
   int i;


   MFreePtr(ct->classTotals);
 
   for(i = VLLength(ct->bins) - 1 ; i >= 0 ; i--) {
      BinFree(VLRemove(ct->bins, i));
   }

   VLFree(ct->bins);
   MFreePtr(ct);
}

void printBins( VoidListPtr bins ) {
  int i;
  for( i = 0; i < VLLength( bins ); i++ ) {
    BinPtr curBin = VLIndex( bins, i );
    printf( "bin %d: %f - %f, (%f), bc - %ld\n", 
	    i, curBin->lowerBound, curBin->upperBound, 
	    curBin->exampleCount, curBin->boundryCount );
  }
}

void ContinuousTrackerRemoveExample(ContinuousTrackerPtr ct,
				    float value, int theclass) {
  int i, index, found;

  int min, max;

  BinPtr bin;
  int numBins = VLLength(ct->bins);

  

  DebugError(ct->initMinMax == 0, "Trying to remove an example from an uninitialized continuous tracker");

  // Find the bin holding the example to be forgotten
  found = 0; index = 0;
  min = 0;
  max = numBins - 1;

  // Binary search
  while(min <= max && !found) {
    i = (min + max) / 2;
    bin = VLIndex(ct->bins, i);
    if((value >= bin->lowerBound && value < bin->upperBound)
        || ((i == numBins - 1) && (value >= bin->lowerBound) 
                        && (value <= bin->upperBound))) {
      found = 1;
      index = i;
    } else if(value < bin->lowerBound) {
      max = i - 1;
    } else {
      min = i + 1;
    }
  }
  
  // Reduce counts
  if (found != 1) {
    //printf("Value %f not found\n", value );
    //printBins( ct->bins );
    // HERE UNREPORTED ERROR

    return;
  }

  bin = VLIndex(ct->bins, index);
  /*
  printf("Value: %f, found bin %d: %f - %f (%f, %f)\n", value, index, 
	 bin->lowerBound, bin->upperBound, bin->exampleCount, 
	 bin->boundryCount); 
  */
  ct->classTotals[theclass]--;
  if(ct->classTotals[theclass] < 0) {
    ct->classTotals[theclass] = 0;
  }
  ct->exampleCount--;
  DebugError(ct->exampleCount < 0, "Continuous tracker has negative examples");
  if(ct->exampleCount < 0) {
    ct->exampleCount = 0;
  }

  // decrement counts
  bin->classTotals[theclass] -= 1;
  if(bin->classTotals[theclass] < 0){
    bin->classTotals[theclass] = 0;
  }

  bin->exampleCount -= 1;
  if ( bin->exampleCount < 0 ) {
    bin->exampleCount = 0;
  }

  if(bin->boundryClass == theclass) {
    bin->boundryCount--;
  }
  if(bin->boundryCount < 0) {
    bin->boundryCount = 0;
  }

  // Remove bin
  if(bin->boundryCount == 0 && ct->addingNewBins) {
    int numClasses = ExampleSpecGetNumClasses(ct->spec);
    if (index != 0) {
      int j;
      BinPtr prevBin = VLIndex(ct->bins, index - 1);
      /*
      printf("Merging bin %d: %f - %f (%f) bc - %ld  with %d: %f - %f (%f) bc - %ld\n",
	     index, bin->lowerBound, bin->upperBound, bin->exampleCount, bin->boundryCount, (index - 1),
	     prevBin->lowerBound, prevBin->upperBound, prevBin->exampleCount, prevBin->boundryCount ); 
      */
      prevBin->upperBound = bin->upperBound;
      prevBin->exampleCount += bin->exampleCount;
      for(j = 0; j < numClasses; j++) {
	prevBin->classTotals[j] += bin->classTotals[j];
      }
      
      VLRemove(ct->bins, index);
      BinFree(bin);
      bin = VLIndex(ct->bins, index - 1);
      /*
      printf( "New bin %d: %f - %f (%f) bc - %ld\n", index - 1, bin->lowerBound,
	      bin->upperBound, bin->exampleCount, bin->boundryCount );
      */
      // Replace max if last bin was removed
      if(index != numBins - 1) {
	BinPtr newLastBin = VLIndex(ct->bins, index);
	ct->max = newLastBin->upperBound;
      }
    }
    // Remove first bin
    else if (VLLength(ct->bins) > 1) {
      int j;
      BinPtr newFirstBin = VLIndex(ct->bins, 1);
      BinPtr oldFirstBin = VLIndex(ct->bins, 0);
      /*
      printf( "removing first bin: %f - %f (%f) bc - %ld\n", oldFirstBin->lowerBound,
	      oldFirstBin->upperBound, oldFirstBin->exampleCount, oldFirstBin->boundryCount);
      */
      newFirstBin->lowerBound = oldFirstBin->lowerBound;
      newFirstBin->exampleCount += oldFirstBin->exampleCount;
      for(j = 0; j < numClasses; j++) {
	newFirstBin->classTotals[j] += oldFirstBin->classTotals[j];
      }
      /*
      printf( "new first bin: %f - %f (%f) bc - %ld\n", newFirstBin->lowerBound,
	      newFirstBin->upperBound, newFirstBin->exampleCount, newFirstBin->boundryCount);
      */
      VLRemove(ct->bins, 0);
      BinFree(oldFirstBin);
    }
  }
}

void ContinuousTrackerAddExample(ContinuousTrackerPtr ct, 
                                    float value, int theclass) {
   int i, index, found;
   int last, first;
   int min, max;
   float percent;
   BinPtr bin, newBin;
   int numBins = VLLength(ct->bins);



   if(!ct->initMinMax) {
      ct->initMinMax = 1;
      ct->min = value;
      ct->max = value;
   }

   if(value < ct->min) {
      ct->min = value;
   } else if(value > ct->max) {
      ct->max = value;
   }

   ct->classTotals[theclass]++;
   ct->exampleCount++;

   if(VLLength(ct->bins) == 0) {
      /* just create a new bin */
      newBin = BinNew(ct->spec);
      newBin->classTotals[theclass]++;
      newBin->exampleCount = 1;
      newBin->boundryClass = theclass;
      newBin->boundryCount = 1;
      newBin->upperBound = value;
      newBin->lowerBound = value;

      VLAppend(ct->bins, newBin);
   } else {
      /* find the bin nearest where the new example would fall */
      found = 0;
      min = 0;
      max = VLLength(ct->bins) - 1;
      index = 0;
      /* binary search for the place to stick the example */
      while(min <= max && !found) {
         i = (min + max) / 2;
         bin = VLIndex(ct->bins, i);
         if((value >= bin->lowerBound && value < bin->upperBound)
           || ((i == numBins - 1) && (value >= bin->lowerBound) 
                        && (value <= bin->upperBound))) {
            found = 1;
            index = i;
         } else if(value < bin->lowerBound) {
            max = i - 1;
         } else {
            min = i + 1;
         }

      //found = 0;
      //for(i = 0 ; i < VLLength(ct->bins) && !found ; i++) {
      //   bin = VLIndex(ct->bins, i);
      //   if(value >= bin->lowerBound && value < bin->upperBound) {
      //      found = 1;
      //      index = i;
      //   }

      }

      first = last = 0;
      if(!found) {
         /* check if it goes before the first bin or after the last one */
         bin = VLIndex(ct->bins, 0);
         if(bin->lowerBound > value) {
            /* go before the first bin */
            index = 0;
            first = 1;
         } else {
            /* if we haven't found it yet value must be > last bins upperBound */
            index = VLLength(ct->bins) - 1;
            last = 1;
         }
      }

      bin = VLIndex(ct->bins, index);

      /* if this is the exact same boundary and class as the bin boundary 
            or we aren't adding new bins any more
                then increment boundary counts */
      if(bin->lowerBound == value || !ct->addingNewBins) {
         bin->classTotals[theclass]++;
         bin->exampleCount++;
         if(bin->boundryClass == theclass && bin->lowerBound == value) {
            /* if it is also the same class then special case it */
            bin->boundryCount++;
         }
      } else {
         /* create a new bin */
         newBin = BinNew(ct->spec);
         newBin->classTotals[theclass]++;
         newBin->boundryCount = 1;
         newBin->boundryClass = theclass;
         newBin->exampleCount = 1;
         newBin->upperBound = bin->upperBound;
         newBin->lowerBound = value;

         /* estimate initial counts  with a linear interpolation */
         //if(last) {
         //   bin->upperBound = ct->max;
         //} else if(index == 0) {
         //   bin->lowerBound = ct->min;
         //}

         if(bin->upperBound - bin->lowerBound == 0 ||
              last || first) {
            percent = 0;
         } else {
            percent = 1.0 - ((value - bin->lowerBound) /
                       (bin->upperBound - bin->lowerBound));
         }

         /* take out the boundry points, they stay with the old bin */
         bin->classTotals[bin->boundryClass] -= bin->boundryCount;
         bin->exampleCount -= bin->boundryCount;

         for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
            newBin->classTotals[i] += bin->classTotals[i] * percent;
            bin->classTotals[i] -= bin->classTotals[i] * percent;
         }

         newBin->exampleCount += bin->exampleCount * percent;
         bin->exampleCount -= bin->exampleCount * percent;

         /* put the boundry examples back in */
         bin->classTotals[bin->boundryClass] += bin->boundryCount;
         bin->exampleCount += bin->boundryCount;

         /* insert the new bin in the right place */
         if(last) {
            bin->upperBound = value;
            newBin->upperBound = value;
            VLAppend(ct->bins, newBin);
         } else if(first) {
            newBin->upperBound = bin->lowerBound;
            VLInsert(ct->bins, newBin, 0);
         } else {
            newBin->upperBound = bin->upperBound;
            bin->upperBound = value;
            VLInsert(ct->bins, newBin, index + 1);
         }
      }
   }

/*   printf("-------------current bins--------\n");
count = 0;
   for(i = 0 ; i < VLLength(ct->bins) ; i++) {
      bin = VLIndex(ct->bins, i);
      count += bin->exampleCount;
      printf("   ");
      BinWrite(bin, stdout);
   }
printf("ct count %ld bins sum %.2f\n", ct->exampleCount, count);
   ContinuousTrackerEntropyAttributeSplit(ct, &index1, &thresh1, &index2, &thresh2);
   printf("i1: %.2f t1: %.2f i2: %.2f t2: %.2f\n", index1, thresh1, index2, thresh2);

   printf("--------------------------------\n");*/
}

/* HERE there is a bug when not adding new boundaries because I don't
change the bottom bin's lowerbound appropriately */

float ContinuousTrackerGetPercentBelowThreshold(ContinuousTrackerPtr ct,
                                                      float thresh) {
   int i, found, index;

   BinPtr bin;
   long seen;
   float percent;

   if(ct->exampleCount == 0 || VLLength(ct->bins) == 0) {
      return 1.0;
   } else {
      /* check for before first or after last */
      bin = VLIndex(ct->bins, 0);
      if(thresh < bin->lowerBound) {
         return 0.0;
      }

      bin = VLIndex(ct->bins, VLLength(ct->bins) - 1);
      if(thresh >= bin->upperBound) {
         return 1.0;
      }

      /* nope, now do the interpolation */
      index = 0; seen = 0;
      found = 0;
      for(i = 0 ; i < VLLength(ct->bins) && !found ; i++) {
         bin = VLIndex(ct->bins, i);
         if(thresh >= bin->lowerBound && thresh < bin->upperBound) {
            found = 1;
            index = i;
         } else {
            seen += bin->exampleCount;
         }
      }

      bin = VLIndex(ct->bins, index);

      /* estimate extra counts */
      if(index == VLLength(ct->bins) - 1) {
         bin->upperBound = ct->max;
      } else if(index == 0) {
         bin->lowerBound = ct->min;
      }

      if(bin->upperBound - bin->lowerBound == 0) {
         percent = 0;
      } else {
         percent = (thresh - bin->lowerBound) /
                    (bin->upperBound - bin->lowerBound);
      }

      /* make sure the boundry examples get counted towards below */
      seen += (bin->exampleCount - bin->boundryCount) * percent;
      seen += bin->boundryCount;

      return seen / ct->exampleCount;
   }
}

void ContinuousTrackerEntropyAttributeSplit(ContinuousTrackerPtr ct,
                        float *firstIndex, float *firstThresh,
                        float *secondIndex, float *secondThresh) {
   float totalEntropy, belowEntropy, aboveEntropy;
   float belowCount, aboveCount;
   float *belowTotals, *aboveTotals;
   int i, j;
   float prob;
   float mdlCost;
   float thisThresh;
   BinPtr bin;

   if(VLLength(ct->bins) - 1 < 1) {
      mdlCost = 0;
   } else {
      mdlCost = lg(VLLength(ct->bins) - 1) / ct->exampleCount;
   }
   if(mdlCost < 0) { mdlCost = 0; }

   *firstIndex = 1000;
   *secondIndex = 1000;

   /* initialize the counts */
   belowCount = 0;
   aboveCount = ct->exampleCount;
   belowTotals = _MakeClassCounts(ct->spec);
   aboveTotals = _MakeClassCounts(ct->spec);
   for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
      belowTotals[i] = 0;
      aboveTotals[i] = ct->classTotals[i];
   }

//printf("num bins %d\n", VLLength(ct->bins));
//printf("num samples %d\n", ct->exampleCount);

   for(i = 0 ; i < (VLLength(ct->bins) - 1) ; i++) {
      bin = VLIndex(ct->bins, i);

      /* move the contents of that bin from above to below */
      belowCount += bin->exampleCount;
      aboveCount -= bin->exampleCount;
      for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         belowTotals[j] += bin->classTotals[j];
         aboveTotals[j] -= bin->classTotals[j];
      }

      belowEntropy = 0.0;
      if(belowCount > 0) {
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            if(belowTotals[j] > 0) {
               prob = belowTotals[j] / belowCount;
               belowEntropy += -(prob * lg(prob));
            }
         }
      }

      aboveEntropy = 0.0;
      if(aboveCount > 0) {
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            if(aboveTotals[j] > 0) {
               prob = aboveTotals[j] / aboveCount;
               aboveEntropy += -(prob * lg(prob));
            }
         }
      }

      totalEntropy = (belowCount / ((float)(ct->exampleCount))) * 
              belowEntropy + 
                    (aboveCount / ((float)(ct->exampleCount))) * 
              aboveEntropy;

      totalEntropy += mdlCost;

      if(bin->exampleCount == 1) {
         thisThresh = (bin->lowerBound + bin->upperBound) / 2;
      } else {
         thisThresh = bin->upperBound;
      }

      /* now see if that is better than what we had before */
      if(totalEntropy < *firstIndex) {
         *secondIndex = *firstIndex;
         *secondThresh = *firstThresh;
         *firstIndex = totalEntropy;
         *firstThresh = thisThresh;
      } else if(totalEntropy < *secondIndex) {
         *secondIndex = totalEntropy;
         *secondThresh = thisThresh;
      }
   }

   MFreePtr(belowTotals);
   MFreePtr(aboveTotals);
}

int ContinuousTrackerNumSplitThresholds(ContinuousTrackerPtr ct) {
   return VLLength(ct->bins);
}

int ContinuousTrackerGetMostCommonClassInPartition(ContinuousTrackerPtr ct,
                        float threshold, int above) {
   float *belowTotals, *aboveTotals, percent;
   int i, j;
   BinPtr bin;
   int commonClass, commonCount;

   /* initialize the counts */
   belowTotals = _MakeClassCounts(ct->spec);
   aboveTotals = _MakeClassCounts(ct->spec);
   for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
      belowTotals[i] = 0;
      aboveTotals[i] = ct->classTotals[i];
   }

   i = 0;
   bin = VLIndex(ct->bins, i);
   while(i < (VLLength(ct->bins) - 1) && threshold > bin->upperBound) {
      /* move the contents of that bin from above to below */
      for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         belowTotals[j] += bin->classTotals[j];
         aboveTotals[j] -= bin->classTotals[j];
      }

      i++;
      bin = VLIndex(ct->bins, i);
   }

   /* now the bin should be set to the one that contains the threshold,
           so do interpolation if needed */

   /* estimate extra counts */
   if(i == (VLLength(ct->bins) - 1)) {
      bin->upperBound = ct->max;
   } else if(i == 0) {
      bin->lowerBound = ct->min;
   }

   if(bin->upperBound - bin->lowerBound == 0) {
      percent = 0;
   } else {
      percent = (threshold - bin->lowerBound) /
                 (bin->upperBound - bin->lowerBound);
   }

   /* HERE I can do better interpolation by forcing the boundary below */
   for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
      belowTotals[j] += (bin->classTotals[j] * percent);
      aboveTotals[j] -= (bin->classTotals[j] * percent);
   }
   
   /* now find the most common class in the appropriate list */
   if(above) {
      commonClass = 0;
      commonCount = aboveTotals[0];
      for(j = 1 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         if(aboveTotals[j] > commonCount) {
            commonClass = j;
            commonCount = aboveTotals[j];
         }
      }
   } else {
      commonClass = 0;
      commonCount = belowTotals[0];
      for(j = 1 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         if(belowTotals[j] > commonCount) {
            commonClass = j;
            commonCount = belowTotals[j];
         }
      }
   }
   MFreePtr(belowTotals);
   MFreePtr(aboveTotals);

   return commonClass;
}



void ContinuousTrackerGiniAttributeSplit(ContinuousTrackerPtr ct,
                        float *firstIndex, float *firstThresh,
                        float *secondIndex, float *secondThresh) {
   float totalGini, belowGini, aboveGini;
   float belowCount, aboveCount;
   float *belowTotals, *aboveTotals;
   int i, j;
   float prob;
   BinPtr bin;

   *firstIndex = 1000;
   *secondIndex = 1000;

   /* initialize the counts */
   belowCount = 0;
   aboveCount = ct->exampleCount;
   belowTotals = _MakeClassCounts(ct->spec);
   aboveTotals = _MakeClassCounts(ct->spec);
   for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
      aboveTotals[i] = ct->classTotals[i];
   }

   for(i = 0 ; i < (VLLength(ct->bins) - 1) ; i++) {
      bin = VLIndex(ct->bins, i);

      /* move the contents of that bin from above to below */
      belowCount += bin->exampleCount;
      aboveCount -= bin->exampleCount;
      for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         belowTotals[j] += bin->classTotals[j];
         aboveTotals[j] -= bin->classTotals[j];
      }

      if(belowCount > 0 && aboveCount > 0) {
         belowGini = 1.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            prob = belowTotals[j] / belowCount;
            belowGini -= (prob * prob);
         }
         aboveGini = 0.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            if(aboveTotals[j] > 0) {
               prob = aboveTotals[j] / aboveCount;
               aboveGini -= (prob * prob);
            }
         }

         totalGini = (belowCount / ((float)(ct->exampleCount))) * 
                 belowGini + 
                       (aboveCount / ((float)(ct->exampleCount))) * 
                 aboveGini;

         /* now see if that is better than what we had before */
         if(totalGini < *firstIndex) {
            *secondIndex = *firstIndex;
            *secondThresh = *firstThresh;
            *firstIndex = totalGini;
            *firstThresh = bin->lowerBound;
         } else if(totalGini < *secondIndex) {
            *secondIndex = totalGini;
            *secondThresh = bin->lowerBound;
         }
      }
   }
   MFreePtr(belowTotals);
   MFreePtr(aboveTotals);
}

void ContinuousTrackerDisableWorseThanEntropy(ContinuousTrackerPtr ct, 
                                                   float entropyThresh) {
   float totalEntropy, belowEntropy, aboveEntropy;
   float belowCount, aboveCount;
   float *belowTotals, *aboveTotals;
   int i, j;
   float prob;
   BinPtr bin, nextBin;

   /* initialize the counts */
   belowCount = 0;
   aboveCount = ct->exampleCount;
   belowTotals = _MakeClassCounts(ct->spec);
   aboveTotals = _MakeClassCounts(ct->spec);
   for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
      aboveTotals[i] = ct->classTotals[i];
   }

   for(i = 0 ; i < (VLLength(ct->bins) - 1) ; i++) {
      bin = VLIndex(ct->bins, i);

      /* move the contents of that bin from above to below */
      belowCount += bin->exampleCount;
      aboveCount -= bin->exampleCount;
      for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         belowTotals[j] += bin->classTotals[j];
         aboveTotals[j] -= bin->classTotals[j];
      }

      if(belowCount > 0 && aboveCount > 0) {
         belowEntropy = 0.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            if(belowTotals[j] > 0) {
               prob = belowTotals[j] / belowCount;
               belowEntropy += -(prob * lg(prob));
            }
         }
         aboveEntropy = 0.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            if(aboveTotals[j] > 0) {
               prob = aboveTotals[j] / aboveCount;
               aboveEntropy += -(prob * lg(prob));
            }
         }

         totalEntropy = (belowCount / ((float)(ct->exampleCount))) * 
                 belowEntropy + 
                       (aboveCount / ((float)(ct->exampleCount))) * 
                 aboveEntropy;

         /* now see if that is better than the minimum threshold */
         if(totalEntropy > entropyThresh) {
//printf("disabled a bad split point\n");
            /* disable it */
            nextBin = VLIndex(ct->bins, i + 1);
            nextBin->lowerBound = bin->lowerBound;
            for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
               nextBin->classTotals[j] += bin->classTotals[j];
               aboveTotals[j] += bin->classTotals[j];
               belowTotals[j] -= bin->classTotals[j];

            }
            nextBin->exampleCount += bin->exampleCount;
            BinFree(VLRemove(ct->bins, i));
            /* now change the index so we consider the correct split next */
            i--;
         }
      }
   }

   MFreePtr(belowTotals);
   MFreePtr(aboveTotals);
}

void ContinuousTrackerDisableWorseThanGini(ContinuousTrackerPtr ct,
					 float giniThresh) {
   float totalGini, belowGini, aboveGini;
   float belowCount, aboveCount;
   float *belowTotals, *aboveTotals;
   int i, j;
   float prob;
   BinPtr bin, nextBin;

   /* initialize the counts */
   belowCount = 0;
   aboveCount = ct->exampleCount;
   belowTotals = _MakeClassCounts(ct->spec);
   aboveTotals = _MakeClassCounts(ct->spec);
   for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
      aboveTotals[i] = ct->classTotals[i];
   }

   for(i = 0 ; i < (VLLength(ct->bins) - 1) ; i++) {
      bin = VLIndex(ct->bins, i);

      /* move the contents of that bin from above to below */
      belowCount += bin->exampleCount;
      aboveCount -= bin->exampleCount;
      for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
         belowTotals[j] += bin->classTotals[j];
         aboveTotals[j] -= bin->classTotals[j];
      }

      if(belowCount > 0 && aboveCount > 0) {
         belowGini = 1.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            prob = belowTotals[j] / belowCount;
            belowGini -= (prob * prob);
         }
         aboveGini = 0.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            if(aboveTotals[j] > 0) {
               prob = aboveTotals[j] / aboveCount;
               aboveGini -= (prob * prob);
            }
         }

         totalGini = (belowCount / ((float)(ct->exampleCount))) * 
                 belowGini + 
                       (aboveCount / ((float)(ct->exampleCount))) * 
                 aboveGini;

         /* now see if that is better than the minimum threshold */
         if(totalGini > giniThresh) {
            /* disable it */
            nextBin = VLIndex(ct->bins, i + 1);
            nextBin->lowerBound = bin->lowerBound;
            for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
               nextBin->classTotals[j] += bin->classTotals[j];
               aboveTotals[j] += bin->classTotals[j];
               belowTotals[j] -= bin->classTotals[j];

            }
            nextBin->exampleCount += bin->exampleCount;
            BinFree(VLRemove(ct->bins, i));
            /* now change the index so we consider the correct split next */
            i--;
         }
      }
   }
   MFreePtr(belowTotals);
   MFreePtr(aboveTotals);
}

static float _CalculateSplitG(BinPtr bin, float splitIndex) {
   /* HERE I may want to add in some weight for how long the split
      point has been around, giving new split points a chance to 
      mature */
   return splitIndex;
}



int ContinuousTrackerPruneSplitsEntropy(ContinuousTrackerPtr ct,
                               int maxSplits, int pruneDownTo) {
   int numToPrune = 0;
   float *Gs;
   int   *indexes;
   int i, j;

   int tmpIndex, thisIndex;
   float tmpG, thisG;

   float totalEntropy, belowEntropy, aboveEntropy;
   float belowCount, aboveCount;
   float *belowTotals, *aboveTotals;
   float prob;
   BinPtr bin, dstBin;

   if(VLLength(ct->bins) > maxSplits) {
      ct->timesPrunedCount++;
      //if(ct->timesPrunedCount > 5) {
         /* HERE make this a parameter.  If we pruned 5 times that is enough */
      //   ct->addingNewBins = 0;
      //}
      numToPrune = VLLength(ct->bins) - pruneDownTo;

      //if(!ct->addingNewBins) {
      //   /* keep a few extra if we aren't going to add any more */
      //   numToPrune /= 2;
      //}

      Gs = MNewPtr(sizeof(float) * numToPrune);
      indexes = MNewPtr(sizeof(int) * numToPrune);


      for(i = 0 ; i < numToPrune ; i++) {
         Gs[i] = 0;
         indexes[i] = -1;
      }

      /* initialize the entropy counts */
      belowCount = 0;
      aboveCount = ct->exampleCount;
      belowTotals = _MakeClassCounts(ct->spec);
      aboveTotals = _MakeClassCounts(ct->spec);
      for(i = 0 ; i < ExampleSpecGetNumClasses(ct->spec) ; i++) {
         belowTotals[i] = 0;
         aboveTotals[i] = ct->classTotals[i];
      }

      for(i = 0 ; i < (VLLength(ct->bins) - 1) ; i++) {
         bin = VLIndex(ct->bins, i);

         /* move the contents of that bin from above to below */
         belowCount += bin->exampleCount;
         aboveCount -= bin->exampleCount;
         for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
            belowTotals[j] += bin->classTotals[j];
            aboveTotals[j] -= bin->classTotals[j];
         }

         if(belowCount > 0 && aboveCount > 0) {
            belowEntropy = 0.0;
            for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
               if(belowTotals[j] > 0) {
                  prob = belowTotals[j] / belowCount;
                  belowEntropy += -(prob * lg(prob));
               }
            }
            aboveEntropy = 0.0;
            for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
               if(aboveTotals[j] > 0) {
                  prob = aboveTotals[j] / aboveCount;
                  aboveEntropy += -(prob * lg(prob));
               }
            }

            totalEntropy = (belowCount / ((float)(ct->exampleCount))) * 
                    belowEntropy + 
                          (aboveCount / ((float)(ct->exampleCount))) * 
                    aboveEntropy;

            /* now figure out if this is 'better' than what we have */
            /* to ditch this split point we need to remove the next bin */
            bin = VLIndex(ct->bins, i + 1);
            thisG = _CalculateSplitG(bin, totalEntropy);

            thisIndex = i + 1;
            for(j = 0 ; j < numToPrune ; j++) {
               if(thisG >= Gs[j]) {
                  tmpG = Gs[j];
                  Gs[j] = thisG;
                  thisG = tmpG;
                  tmpIndex = indexes[j];
                  indexes[j] = thisIndex;
                  thisIndex = tmpIndex;
               }
            }
         } else {
            /* HERE BUG I think there is a bug that triggers this sometimes
                and I think it is related to round off errors with too much
                floating point math so that the aboveCount & belowCount
                get out of wack and one of them goes below 0 when it shouldn't
            */
            //printf("index %d counts were 0 ", i);
            //BinWrite(bin, stdout);
         }
      }

      /* do the deactivations */
      for(i = 0 ; i < numToPrune - 1; i++) {
	int found = 0;
	bin = VLIndex(ct->bins, indexes[i]);
	/*find the dstBin */
	j = indexes[i];
	if (j == -1) 
	  break;
	dstBin = 0;
	while(dstBin == 0) {
	  j--;
	  dstBin = VLIndex(ct->bins, j);
	  found = 1;
	}
	if ( !found )
	  break;
	dstBin->upperBound = bin->upperBound;
	for(j = 0 ; j < ExampleSpecGetNumClasses(ct->spec) ; j++) {
	  dstBin->classTotals[j] += bin->classTotals[j];
	}
	dstBin->exampleCount += bin->exampleCount;

//printf("threw away split point at %.2f with entropy %.2f\n", bin->lowerBound,
//            Gs[i]);
         BinFree(bin);
         VLSet(ct->bins, indexes[i], 0);
         
      }

      /* now remove all the 0s */
      for(i = VLLength(ct->bins) - 1 ; i >= 0 ; i--) {
         if(VLIndex(ct->bins, i) == 0) {
            VLRemove(ct->bins, i);
         }
      }

      MFreePtr(Gs);
      MFreePtr(indexes);
      MFreePtr(belowTotals);
      MFreePtr(aboveTotals);
   }

   return numToPrune;
}


ExampleGroupStatsPtr ExampleGroupStatsNew(ExampleSpecPtr es, 
					  AttributeTrackerPtr at) {
   ExampleGroupStatsPtr egs = MNewPtr(sizeof(ExampleGroupStats));
   int i, j, k;
   int inserted;
   long *array;
   VoidAListPtr tmpList;


   egs->spec = es;
   egs->attributeTracker = at;

   egs->examplesSeen = 0;

   egs->classTotals = MNewPtr(sizeof(long) * 
                        ExampleSpecGetNumClasses(egs->spec));
   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      egs->classTotals[i] = 0;
   }
   egs->stats = VALNew();

   for(i = 0 ; i < ExampleSpecGetNumAttributes(egs->spec) ; i++) {
      inserted = 0;
      if(AttributeTrackerIsActive(egs->attributeTracker, i)) {
         switch(ExampleSpecGetAttributeType(egs->spec, i)) {
          case asDiscreteNamed:
          case asDiscreteNoName:
            tmpList = VALNew();
            for(j = 0 ;
                 j < ExampleSpecGetAttributeValueCount(egs->spec, i) ; j++) {
               k = ExampleSpecGetNumClasses(egs->spec);
               array = MNewPtr(sizeof(long) * k);

               k--; /* zero the array, remember it's 0 indexed */
               while(k >= 0) {
                  array[k] = 0;
                  k--;
               }

               VALAppend(tmpList, array);
            }
            VALAppend(egs->stats, tmpList);
            inserted = 1;
            break;
          case asContinuous:
            VALAppend(egs->stats, ContinuousTrackerNew(egs->spec));
            inserted = 1;
            break;
          case asIgnore:
            break;
	 }
      }
      if(!inserted) {
	/* this stat is inactive or we can't deal with it, put a placeholder */
         VALAppend(egs->stats, 0);
      }
   }

   return egs;
}

void ExampleGroupStatsFree(ExampleGroupStatsPtr egs) {
   int i, j;
   VoidAListPtr tmp;

   for(i = VALLength(egs->stats) - 1 ; i >= 0 ; i--) {
      tmp = VALRemove(egs->stats, i);
      if(AttributeTrackerIsActive(egs->attributeTracker, i) && tmp != 0) {
         switch(ExampleSpecGetAttributeType(egs->spec, i)) {
          case asDiscreteNamed:
          case asDiscreteNoName:
            for(j = VALLength(tmp) - 1 ; j >= 0 ; j--) {
               MFreePtr(VALRemove(tmp, j));
            }
            VALFree(tmp);
            break;
          case asContinuous:
            ContinuousTrackerFree((ContinuousTrackerPtr)tmp);
            break;
          case asIgnore:
            break;
         }
      }
   }
   VALFree(egs->stats);

   AttributeTrackerFree(egs->attributeTracker);
   MFreePtr(egs);
}

void ExampleGroupStatsReactivate(ExampleGroupStatsPtr egs) {
   int i, j, k;
   int inserted;
   long *array;
   VoidAListPtr tmpList;


   egs->examplesSeen = 0;

   egs->classTotals = MNewPtr(sizeof(long) * 
                        ExampleSpecGetNumClasses(egs->spec));
   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      egs->classTotals[i] = 0;
   }
   egs->stats = VALNew();

   for(i = 0 ; i < ExampleSpecGetNumAttributes(egs->spec) ; i++) {
      inserted = 0;
      if(AttributeTrackerIsActive(egs->attributeTracker, i)) {
         switch(ExampleSpecGetAttributeType(egs->spec, i)) {
          case asDiscreteNamed:
          case asDiscreteNoName:
            tmpList = VALNew();
            for(j = 0 ;
                 j < ExampleSpecGetAttributeValueCount(egs->spec, i) ; j++) {
               k = ExampleSpecGetNumClasses(egs->spec);
               array = MNewPtr(sizeof(long) * k);

               k--; /* zero the array, remember it's 0 indexed */
               while(k >= 0) {
                  array[k] = 0;
                  k--;
               }

               VALAppend(tmpList, array);
            }
            VALAppend(egs->stats, tmpList);
            inserted = 1;
            break;
          case asContinuous:
            VALAppend(egs->stats, ContinuousTrackerNew(egs->spec));
            inserted = 1;
            break;
          case asIgnore:
            break;
	 }
      }
      if(!inserted) {
	/* this stat is inactive or we can't deal with it, put a placeholder */
         VALAppend(egs->stats, 0);
      }
   }
}

void ExampleGroupStatsDeactivate(ExampleGroupStatsPtr egs) {
   int i, j;
   VoidAListPtr tmp;

   for(i = VALLength(egs->stats) - 1 ; i >= 0 ; i--) {
      tmp = VALRemove(egs->stats, i);
      if(AttributeTrackerIsActive(egs->attributeTracker, i) && tmp != 0) {
         switch(ExampleSpecGetAttributeType(egs->spec, i)) {
          case asDiscreteNamed:
          case asDiscreteNoName:
            for(j = VALLength(tmp) - 1 ; j >= 0 ; j--) {
               MFreePtr(VALRemove(tmp, j));
            }
            VALFree(tmp);
            break;
          case asContinuous:
            ContinuousTrackerFree((ContinuousTrackerPtr)tmp);
            break;
          case asIgnore:
            break;
         }
      }
   }
   VALFree(egs->stats);
}

void ExampleGroupStatsRemoveExample(ExampleGroupStatsPtr egs, ExamplePtr e) {
  int i;
  VoidListPtr attributeInfo;
  
  /* HERE : what do you do if you don't know a class? */
  if(ExampleIsClassUnknown(e)) { return; }
  
  egs->examplesSeen--;
  
  egs->classTotals[ExampleGetClass(e)]--;
  
  for(i = 0 ; i < ExampleSpecGetNumAttributes(egs->spec) ; i++) {
    if(AttributeTrackerIsActive(egs->attributeTracker, i) &&
       !ExampleIsAttributeUnknown(e, i)) {
      attributeInfo = VLIndex(egs->stats, i);
      if(attributeInfo != 0) {
	switch(ExampleSpecGetAttributeType(egs->spec, i)) {
	case asDiscreteNamed:
	case asDiscreteNoName:
	  ((long *)(VALIndex(attributeInfo,
			     ExampleGetDiscreteAttributeValue(e, i))))
	    [ExampleGetClass(e)]--;
	  
	  break;
	case asContinuous:
	  ContinuousTrackerRemoveExample
	    ((ContinuousTrackerPtr)attributeInfo,
	     ExampleGetContinuousAttributeValue(e, i),
	     ExampleGetClass(e));
	  break;
        case asIgnore:
          break;
	}
      }
    }
  }
}

void ExampleGroupStatsAddExample(ExampleGroupStatsPtr egs, ExamplePtr e) {
   int i;
   VoidListPtr attributeInfo;
   
   /* HERE : what do you do if you don't know a class? */
   if(ExampleIsClassUnknown(e)) { return; }

   egs->examplesSeen++;

   egs->classTotals[ExampleGetClass(e)]++;

   for(i = 0 ; i < ExampleSpecGetNumAttributes(egs->spec) ; i++) {
      if(AttributeTrackerIsActive(egs->attributeTracker, i) &&
            !ExampleIsAttributeUnknown(e, i)) {
         attributeInfo = VLIndex(egs->stats, i);
         if(attributeInfo != 0) {
            switch(ExampleSpecGetAttributeType(egs->spec, i)) {
             case asDiscreteNamed:
             case asDiscreteNoName:
               ((long *)(VALIndex(attributeInfo,
                      ExampleGetDiscreteAttributeValue(e, i))))
                                [ExampleGetClass(e)]++;
               break;
             case asContinuous:
               ContinuousTrackerAddExample((ContinuousTrackerPtr)attributeInfo,
                       ExampleGetContinuousAttributeValue(e, i),
                       ExampleGetClass(e));
               break;
             case asIgnore:
               break;
            }
	 }
      }
   }
}


void ExampleGroupStatsWrite(ExampleGroupStatsPtr egs, FILE *out) {
   int i, j, k;
   long *array;
   VoidAListPtr tmpList;

   fprintf(out, "Class totals:");
   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      fprintf(out, " %ld", egs->classTotals[i]);
   }
   fprintf(out, "\n");

   for(i = 0 ; i < VALLength(egs->stats) ; i++) {
      fprintf(out, "%s\n", ExampleSpecGetAttributeName(egs->spec, i));
      tmpList = VALIndex(egs->stats, i);
      if(tmpList != 0) {
         switch(ExampleSpecGetAttributeType(egs->spec, i)) {
          case asDiscreteNamed:
          case asDiscreteNoName:
            for(j = 0 ; j < VALLength(tmpList) ; j++) {
               array = VALIndex(tmpList, j);
               fprintf(out, "   %s:", 
                  ExampleSpecGetAttributeValueName(egs->spec, i, j));
               for(k = 0 ; k < ExampleSpecGetNumClasses(egs->spec) ; k++) {
                  fprintf(out, " %ld", array[k]);
               }
               fprintf(out, "\n");
            }
            break;
          case asContinuous:
            fprintf(out, "   Continuous attribute\n");
            break;
          case asIgnore:
            fprintf(out, "   Ignored attribute\n");
            break;
         }
      }
   }
}

long ExampleGroupStatsNumExamplesSeen(ExampleGroupStatsPtr egs) {
   return egs->examplesSeen;
}

AttributeTrackerPtr ExampleGroupStatsGetAttributeTracker(
					 ExampleGroupStatsPtr egs) {
   return egs->attributeTracker;
}

int ExampleGroupStatsIsAttributeActive(
				 ExampleGroupStatsPtr egs, int num) {
   return VALIndex(egs->stats, num) != 0;
}

void ExampleGroupStatsIgnoreAttribute(ExampleGroupStatsPtr egs, int num) {
   VoidAListPtr tmp;
   int i;

   tmp = VALIndex(egs->stats, num);
   if(tmp != 0) {
      switch(ExampleSpecGetAttributeType(egs->spec, num)) {
       case asDiscreteNamed:
       case asDiscreteNoName:
         for(i = VALLength(tmp) - 1 ; i >= 0 ; i--) {
            MFreePtr(VALRemove(tmp, i));
         }
         VALFree(tmp);
         VALSet(egs->stats, num, 0);
         break;
       case asContinuous:
         ContinuousTrackerFree((ContinuousTrackerPtr)tmp);
         break;
       case asIgnore:
         break;
      }
      VALSet(egs->stats, num, 0);
   }
}

int ExampleGroupStatsGetMostCommonClass(ExampleGroupStatsPtr egs) {
   int maxIndex = -1;
   long maxVal = -1;
   int i = 0;
   
   while(i < ExampleSpecGetNumClasses(egs->spec)) {
      if(egs->classTotals[i] > maxVal) {
         maxVal = egs->classTotals[i];
         maxIndex = i;
      }
      i++;
   }

   return maxIndex;
}

int ExampleGroupStatsGetMostCommonClassLaplace(ExampleGroupStatsPtr egs, 
      int addClass, int addCount) {
   int maxIndex = -1;
   long maxVal = -1;
   long currentVal;
   int i = 0;
   
   while(i < ExampleSpecGetNumClasses(egs->spec)) {
      currentVal = egs->classTotals[i];
      if(i == addClass) { currentVal += addCount; }

      if(currentVal > maxVal) {
         maxVal = currentVal;
         maxIndex = i;
      }
      i++;
   }

   return maxIndex;
}

long ExampleGroupStatsGetMostCommonClassCount(ExampleGroupStatsPtr egs) {
   int maxIndex = -1;
   long maxVal = -1;
   int i = 0;
   
   while(i < ExampleSpecGetNumClasses(egs->spec)) {
      if(egs->classTotals[i] > maxVal) {
         maxVal = egs->classTotals[i];
         maxIndex = i;
      }
      i++;
   }

   return maxVal;
}

int ExampleGroupStatsGetMostCommonClassForAttVal(ExampleGroupStatsPtr egs,
						 int att, int val) {
   long *counts = (long *)VALIndex(VALIndex(egs->stats, att), val);
   int i, best;

   best = 0;
   for(i = 1 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      if(counts[i] > counts[best]) {
         best = i;
      }
   }

   return best;
}


int ExampleGroupStatsIsPure(ExampleGroupStatsPtr egs) {
   int num = 0;
   int i;

   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      if(egs->classTotals[i] > 0) {
         num++;
      }
   }

   return num <= 1;
}

float ExampleGroupStatsGetValuePercent(ExampleGroupStatsPtr egs, 
				       int attNum, int valNum) {
   int i;
   long sum = 0;
   VoidAListPtr values = VALIndex(egs->stats, attNum);
   long *counts = VALIndex(values, valNum);

   /* HERE this may not totally work right with missing attributes */
   /* HERE this better be a discrete attribute */

   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      sum += counts[i];
   }

   if(egs->examplesSeen > 0) {
      return (float)sum / (float)egs->examplesSeen;
   } else {
      return 0;
   }
}

float ExampleGroupStatsGetValueGivenClassPercent(ExampleGroupStatsPtr egs, 
				       int attNum, int valNum, int classNum) {
   VoidAListPtr values = VALIndex(egs->stats, attNum);
   long *counts = VALIndex(values, valNum);

   /* HERE this may not totally work right with missing attributes */
   /* HERE this better be a discrete attribute */

   if(egs->classTotals[classNum] > 0) {
      return (float)counts[classNum] / (float)egs->classTotals[classNum];
   } else {
      return 0;
   }
}

double ExampleGroupStatsGetValueGivenClassMEstimate(ExampleGroupStatsPtr egs, 
                                        int attNum, int valNum, int classNum) {
   VoidAListPtr values = VALIndex(egs->stats, attNum);
   long *counts = VALIndex(values, valNum);
   double f, answer;


   /* HERE this may not totally work right with missing attributes */
   /* HERE this better be a discrete attribute */

   f = 1.0 / (double)egs->examplesSeen;
   answer = ((double)counts[classNum] + f) /
        ((double)egs->classTotals[classNum] + f * VLLength(values));
   return answer;
}

double ExampleGroupStatsGetValueGivenClassMEstimateLogP(ExampleGroupStatsPtr 
           egs, int attNum, int valNum, int classNum) {
   VoidAListPtr values = VALIndex(egs->stats, attNum);
   long *counts = VALIndex(values, valNum);

   /* HERE this may not totally work right with missing attributes */
   /* HERE this better be a discrete attribute */

   return log(counts[classNum] + (1.0 / VLLength(values))) - 
        log(egs->classTotals[classNum] + 1.0);
}

float ExampleGroupStatsGetClassPercent(ExampleGroupStatsPtr egs, int classNum) {
   if(egs->examplesSeen > 0) { 
      return (float)egs->classTotals[classNum] / (float)egs->examplesSeen;
   } else {
      return 0;
   }
}


double ExampleGroupStatsGetClassLogP(ExampleGroupStatsPtr egs, int classNum) {
   if(egs->examplesSeen > 0 && egs->classTotals[classNum] > 0) { 
      return log(egs->classTotals[classNum]) -  log(egs->examplesSeen);
   } else {
      return -1000000000000;
   }
}


float ExampleGroupStatsGetPercentBelowThreshold(ExampleGroupStatsPtr egs,
                                     int attNum, float thresh) {
   return ContinuousTrackerGetPercentBelowThreshold(
                             VALIndex(egs->stats, attNum), thresh);
}

float ExampleGroupStatsEntropyTotal(ExampleGroupStatsPtr egs) {
   float out = 0.0;
   float prob;
   int i;

   if(egs->examplesSeen == 0) {
     /* HERE trying to get a value from no information */
     return lg(ExampleSpecGetNumClasses(egs->spec));
   }

   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      prob = (float)(egs->classTotals[i]) / ((float)egs->examplesSeen);
      if(prob > 0.0) {
         out += -(prob * lg(prob));
      } /* otherwise this doesn't add to the entropy */
   }

   return out;
}

float ExampleGroupStatsEntropyDiscreteAttributeSplit(ExampleGroupStatsPtr egs,
                                                           int attNum) {
   float out = 0.0;
   float thisEntropy;
   float prob;
   int i, j;
   long *array;
   VoidAListPtr valsList;
   long totalWithThatValue;

   valsList = VALIndex(egs->stats, attNum);

   if(valsList == 0 || egs->examplesSeen == 0) {
     /* HERE getting entropy of inactive attribute
        or with no information, what should I do? 
        return the worst possible entropy for now */
      return lg(ExampleSpecGetNumClasses(egs->spec));
   }

   for(i = 0 ; i < VALLength(valsList) ; i++) {
      array = VALIndex(valsList, i);
      totalWithThatValue = 0;

      for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
         totalWithThatValue += array[j];
      }

      if(totalWithThatValue != 0) {
         thisEntropy = 0.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
            if(array[j] > 0) {
               prob = (float)(array[j]) / ((float)totalWithThatValue);
               thisEntropy += -(prob * lg(prob));
            }
         }
         out += (((float)totalWithThatValue) / ((float)(egs->examplesSeen))) * 
                 thisEntropy;
      }
   }
   return out;
}

float ExampleGroupStatsEntropyMinusDiscreteAttributeSplit(ExampleGroupStatsPtr 
                                 egs, int attNum, float delta) {
   float out = 0.0;
   float thisEntropy;
   float prob, bound, totalBound, probM, probP;
   int i, j;
   long *array;
   VoidAListPtr valsList;
   long totalWithThatValue;

   valsList = VALIndex(egs->stats, attNum);

   if(valsList == 0 || egs->examplesSeen == 0) {
     /* HERE getting entropy of inactive attribute
        or with no information, what should I do? 
        return the worst possible entropy for now */
      return lg(ExampleSpecGetNumClasses(egs->spec));
   }

   totalBound = StatHoeffdingBoundOne(1.0, delta, egs->examplesSeen);

   for(i = 0 ; i < VALLength(valsList) ; i++) {
      array = VALIndex(valsList, i);
      totalWithThatValue = 0;

      for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
         totalWithThatValue += array[j];
      }

      if(totalWithThatValue != 0) {
         thisEntropy = 0.0;
         bound = StatHoeffdingBoundTwo(1.0, delta, totalWithThatValue);
         for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
            if(array[j] > 0) {
               prob = (float)(array[j]) / ((float)totalWithThatValue);

               probM = max(prob - bound, 0.000000001);
               probP = min(prob + bound, 1.0);

               thisEntropy += min(-(probM * log(probM)), 
                                  -(probP * log(probP)));
            }
         }
         
         out += max(0.0, (((float)totalWithThatValue) / (float)(egs->examplesSeen)) - bound) * thisEntropy;
      }
   }

   return out;
}

float ExampleGroupStatsEntropyPlusDiscreteAttributeSplit(ExampleGroupStatsPtr 
                                 egs, int attNum, float delta) {
   float out = 0.0;
   float thisEntropy;
   float prob, bound, totalBound;
   int i, j;
   long *array;
   VoidAListPtr valsList;
   long totalWithThatValue;

   valsList = VALIndex(egs->stats, attNum);

   if(valsList == 0 || egs->examplesSeen == 0) {
     /* HERE getting entropy of inactive attribute
        or with no information, what should I do? 
        return the worst possible entropy for now */
      return lg(ExampleSpecGetNumClasses(egs->spec));
   }

   totalBound = StatHoeffdingBoundOne(1.0, delta, egs->examplesSeen);

   for(i = 0 ; i < VALLength(valsList) ; i++) {
      array = VALIndex(valsList, i);
      totalWithThatValue = 0;

      for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
         totalWithThatValue += array[j];
      }

      if(totalWithThatValue != 0) {
         thisEntropy = 0.0;
         bound = StatHoeffdingBoundTwo(1.0, delta, totalWithThatValue);
         for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
            if(array[j] > 0) {
               prob = (float)(array[j]) / ((float)totalWithThatValue);

               if(prob < kOneOverE) {
                  prob += bound;
                  if(prob > kOneOverE) {
                     prob = kOneOverE;
                  }
               } else { /* prob >= kOneOverE */
                  prob -= bound;
                  if(prob < kOneOverE) {
                     prob = kOneOverE;
                  }
               }

               thisEntropy += -(prob * lg(prob));
            }
         }
         
         out += min(1.0, (((float)totalWithThatValue) / (float)(egs->examplesSeen)) + totalBound) * thisEntropy;
      }
   }

   /* do not return a value worse than the upper bound on entropy */
   if(out > lg(ExampleSpecGetNumClasses(egs->spec))) {
      return lg(ExampleSpecGetNumClasses(egs->spec));
   } else {
      return out;
   }
}

void ExampleGroupStatsEntropyContinuousAttributeSplit(ExampleGroupStatsPtr egs, 
                 int attNum, float *firstIndex, float *firstThresh,
                         float *secondIndex, float *secondThresh) {
   ContinuousTrackerPtr ct;   

   /* just in case, set them to something really really bad to start */
   *firstIndex = 1000;
   *secondIndex = 1000;

   ct = VALIndex(egs->stats, attNum);

   if(ct == 0 || egs->examplesSeen == 0) {
     /* HERE getting entropy of inactive attribute, what should I do? 
        return with the bad initial settings of 1000 */
      return;
   } else {
      ContinuousTrackerEntropyAttributeSplit(ct, firstIndex, firstThresh,
                                secondIndex, secondThresh);
   }
}


float ExampleGroupStatsGiniTotal(ExampleGroupStatsPtr egs) {
   float out = 1.0;
   float tmp;
   int i;

   if(egs->examplesSeen == 0) {
     /* HERE trying to get a gini from no information */
     return 1.0;
   }

   for(i = 0 ; i < ExampleSpecGetNumClasses(egs->spec) ; i++) {
      tmp = ((float)egs->classTotals[i]) / ((float)egs->examplesSeen);
      out -= (tmp * tmp);
   }

   return out;
}

float ExampleGroupStatsGiniDiscreteAttributeSplit(ExampleGroupStatsPtr egs,
                                                           int attNum) {
   float out = 0.0;
   float thisGini;
   float tmp;
   int i, j;
   long *array;
   VoidAListPtr valsList;
   long totalWithThatValue;

   valsList = VALIndex(egs->stats, attNum);

   if(valsList == 0 || egs->examplesSeen == 0) {
     /* HERE getting gini of inactive attribute, what should I do? 
        return the worst possible gini for now */
      return 1.0;
   }

   for(i = 0 ; i < VALLength(valsList) ; i++) {
      array = VALIndex(valsList, i);
      totalWithThatValue = 0;

      for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
         totalWithThatValue += array[j];
      }

      if(totalWithThatValue != 0) {
         thisGini = 1.0;
         for(j = 0 ; j < ExampleSpecGetNumClasses(egs->spec) ; j++) {
            tmp = ((float)array[j]) / ((float)totalWithThatValue);
            thisGini -= (tmp * tmp);
         }
         out += (((float)totalWithThatValue) / ((float)egs->examplesSeen)) * 
                 thisGini;
      }
   }
   return out;
}


void ExampleGroupStatsGiniContinuousAttributeSplit(ExampleGroupStatsPtr egs, 
                 int attNum, float *firstIndex, float *firstThresh,
                         float *secondIndex, float *secondThresh) {
   ContinuousTrackerPtr ct;   

   /* just in case, set them to something really really bad to start */
   *firstIndex = 1000;
   *secondIndex = 1000;

   ct = VALIndex(egs->stats, attNum);

   if(ct == 0 || egs->examplesSeen == 0) {
     /* HERE getting gini of inactive attribute, what should I do? 
        return with the bad initial settings of 1000 */
      return;
   } else {
      ContinuousTrackerGiniAttributeSplit(ct, firstIndex, firstThresh,
                                secondIndex, secondThresh);
   }
}

void ExampleGroupStatsIgnoreSplitsWorseThanEntropy(ExampleGroupStatsPtr egs,
		         int attNum, float entropyThresh) {
   ContinuousTrackerDisableWorseThanEntropy(VALIndex(egs->stats, attNum), 
                 entropyThresh);
}

void ExampleGroupStatsIgnoreSplitsWorseThanGini(ExampleGroupStatsPtr egs,
			int attNum, float giniThresh) {
   ContinuousTrackerDisableWorseThanGini(VALIndex(egs->stats, attNum), 
                 giniThresh);
}

int ExampleGroupStatsLimitSplitsEntropy(ExampleGroupStatsPtr egs, int attNum,
                               int maxSplits, int pruneDownTo) {

   ContinuousTrackerPtr ct = VALIndex(egs->stats, attNum);
   return ContinuousTrackerPruneSplitsEntropy(ct, maxSplits, pruneDownTo);
}

void ExampleGroupStatsStopAddingSplits(ExampleGroupStatsPtr egs, int attNum) {
   ContinuousTrackerPtr ct = VALIndex(egs->stats, attNum);
   ct->addingNewBins = 0;
}

int ExampleGroupStatsNumSplitThresholds(ExampleGroupStatsPtr egs, int attNum) {
   ContinuousTrackerPtr ct = VALIndex(egs->stats, attNum);
   return ContinuousTrackerNumSplitThresholds(ct);
}

int ExampleGroupStatsGetMostCommonClassAboveThreshold(ExampleGroupStatsPtr egs,
                      int attNum, float threshold) {
   ContinuousTrackerPtr ct = VALIndex(egs->stats, attNum);
   return ContinuousTrackerGetMostCommonClassInPartition(ct, threshold, 1);
}

int ExampleGroupStatsGetMostCommonClassBelowThreshold(ExampleGroupStatsPtr egs,
                      int attNum, float threshold) {
   ContinuousTrackerPtr ct = VALIndex(egs->stats, attNum);
   return ContinuousTrackerGetMostCommonClassInPartition(ct, threshold, 0);
}
